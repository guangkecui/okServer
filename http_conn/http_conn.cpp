#include "http_conn.h"
#include <iostream>
#include <sys/mman.h>

using std::cout;
using std::endl;
using std::string;
using std::unordered_map;
int http_conn::m_epollfd = -1;  //静态变量初始化，
int http_conn::m_user_count = 0;//静态变量初始化，

//定义http响应的一些状态信息
const string ok_200_title = "OK";
const string error_400_title = "Bad Request";
const string error_400_form = "Your request has bad syntax or is inherently impossible to staisfy.\n";
const string error_403_title = "Forbidden";
const string error_403_form = "You do not have permission to get file form this server.\n";
const string error_404_title = "Not Found";
const string error_404_form = "The requested file was not found on this server.\n";
const string error_500_title = "Internal Error";
const string error_500_form = "There was an unusual problem serving the request file.\n";

/*将文件描述符设置成非堵塞*/
void http_conn::setnoblock(int fd){
    int old_option = fcntl(fd,F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd,F_SETFL,new_option);
}

/*向epollfd中添加监听的socket
默认LT，*/
void http_conn::addfd(int epollfd,int fd,int isShot,bool ET){
    epoll_event event;
    event.data.fd = fd;
    //EPOLLIN:有新联接或有数据到来，EPOLLRDHUP:对方是否关闭
    //EPOLLONESHOT:信号到达之后，除非重新调用eppol_ctl，否则不会再次唤醒线程，
    //保证一个socket被一个线程处理
    event.events = EPOLLIN | EPOLLRDHUP ;
    if(isShot){
        event.events |= EPOLLONESHOT;
    }
    if(ET){
        event.events |= EPOLLET;
    }
    epoll_ctl(epollfd,EPOLL_CTL_ADD,fd,&event);
    http_conn::setnoblock(fd);
}


/*从内核事件表中删除fd*/
void http_conn::removefd(int epollfd,int fd){
    epoll_ctl(epollfd,EPOLL_CTL_DEL,fd,0);
    close(fd);
}

/*将事件重置为ONESHOT*/
void http_conn::modfd(int epollfd,int fd,int old_option){
    epoll_event event;
    event.data.fd = fd;
    event.events = old_option| EPOLLONESHOT | EPOLLRDHUP;
    epoll_ctl(epollfd,EPOLL_CTL_MOD,fd,&event);
}

/*http关闭函数*/
void http_conn::http_close(){
    if(m_sockfd!=-1){
        //cout << "close socketfd:" << m_sockfd << endl;
        removefd(m_epollfd,m_sockfd);
        m_sockfd = -1;
        m_user_count--;//连接数减1
    }
    if(m_timer!=nullptr){
        m_timer->setDelete();
    }
    unlinktimer();
}

void http_conn::init(int sockfd,const sockaddr_in& addr){
    m_sockfd = sockfd;
    m_address = addr;
    m_user_count++;
    m_timer = nullptr;
    if(!m_name_password.count("123")){
        m_name_password["123"] = "123";
    }
    //cout << "addfd(" << sockfd<< ")" << endl;
    addfd(m_epollfd,sockfd , 1, true); //connectfd为ET，oneshot
    /*关闭sockfd的TIME——WAIT*/
    int mReuseadress = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &mReuseadress, sizeof(mReuseadress));
    init();
}   

void http_conn::process(){
    REQUEST_RESULT read_ret = process_read();
    if(read_ret == NO_REQUEST){
        http_conn::modfd(m_epollfd,m_sockfd,EPOLLIN);
        return;
    }
    bool write_ret = process_write(read_ret);
    if(!write_ret){
        http_close();
    }
    modfd(m_epollfd, m_sockfd, EPOLLOUT);
}

http_conn::REQUEST_RESULT http_conn::master_parse_line(char* text){
    //cout << text << endl;
    m_url = strpbrk(text, " \t"); //在text中找到第一个空格的位置
    if (!m_url)
    {
        return BAD_REQUEST;
    }
    *m_url = '\0';//加入结束符
    m_url++;
    char *method = text; //text指向GET或POST
    if(strcasecmp(method,"GET") == 0){
        m_method = GET;
    }
    else if(strcasecmp(method,"POST") == 0){
        m_method = POST;
    }
    else{
        return BAD_REQUEST;
    }

    m_url += strspn(m_url, " \t"); //将url移动到第一个不是‘\t’的地方;
    m_version = strpbrk(m_url, " \t");//找到下一个空格的位置
    if(!m_version){
        return BAD_REQUEST;
    }
    *m_version = '\0';
    m_version++;
    m_version += strspn(m_version, " \t"); //将mervison移动到不是\t的位置

    if(strcasecmp(m_version,"HTTP/1.1")!=0&&strcasecmp(m_version,"HTTP/1.0")!=0){
        return BAD_REQUEST;
    }

    if(strncasecmp(m_url,"http://",7)==0){
        m_url += 7;
        m_url = strchr(m_url, '/'); //将url指向第一次出现‘/’的地方;
    }
    if(strncasecmp(m_url,"https://",8)==0){
        m_url += 8;
        m_url = strchr(m_url, '/'); //将url指向第一次出现‘/’的地方;
    }
    if(!m_url||m_url[0]!='/'){
        return BAD_REQUEST;
    }
    if (strlen(m_url) == 1)
    {
        strcat(m_url, "homepage.html");
    }
    m_master_state = MASTER_STATE_HEADER;
    return NO_REQUEST;
}

http_conn::REQUEST_RESULT http_conn::master_parse_header(char* text){
    //cout << text << endl;
    /*处于请求头和请求体之间的空行，已被parse_line改成“\0\0”*/
    if(text[0] == '\0'){ 
        if(m_content_length!=0){
            m_master_state = MASTER_STATE_BODY;
            return NO_REQUEST;//此请求为一个POST请求，请求未完成
        }
        else{
            return GET_REQUEST;//获得一个完整的GET请求
        }
    }
    /*处理Connection字段*/
    else if(strncasecmp(text,"Connection:",11)==0){
        text += 11;
        text += strspn(text, " \t");
        if(strcasecmp(text,"keep-alive")==0){
            m_keep_alive = true;
        }
        else{
            m_keep_alive = false;
        }
    }
    /*处理content-length*/
    else if(strncasecmp(text,"Content-Length:",15)==0){
        text += 15;
        text += strspn(text, " \t");
        m_content_length = atoi(text);
    }
    /*处理HOST*/
    else if(strncasecmp(text,"Host:",4)==0){
        text += 5;
        text += strspn(text, " \t");
        m_host = text;
    }
    /*其余暂时不处理*/
    else{
    }
    return NO_REQUEST;
}

http_conn::REQUEST_RESULT http_conn::master_parse_body(char* text){
    if((m_check_index+m_content_length)<=m_read_index){//请求体被完整读入
        text[m_content_length] = '\0';
        m_string = text;
        return GET_REQUEST;
        m_content_length = 0;
    }
    return NO_REQUEST;
}

/*
0:登陆  1：注册    2:注册确认; 3: 注册成功回到主页
*/
http_conn::REQUEST_RESULT http_conn::do_request(){
    if(strcmp(m_url,"/homepag.html")!=0){
        int index = (*(strrchr(m_url, '/')+1))-'0';
        switch (index)
        {
        case 0:{/*点击登录*/
            string name;
            string password;
            process_cgi(name, password);
            cout << "name =" << name << ",password=" << password << endl;
            if(user_is_valid(index,name,password)){
                m_targetfile_path.append("/welcome.html");
            }
            else{
                m_targetfile_path.append("/loginfailed.html");
            }
            break;
        }
        case 1:{/*点击注册*/
            m_targetfile_path.append("/register.html");
            break;
        }
        case 2:{/*注册确认*/
            string name;
            string password;
            process_cgi(name, password);
            cout << "name =" << name << ",password=" << password << endl;
            if(user_is_valid(index,name,password)){
                m_targetfile_path.append("/registerSuccess.html");
            }
            else{
                m_targetfile_path.append("/registerFailed.html");
            }
            break;
        }
        case 3:{/*注册成功回到主页*/
            m_targetfile_path.append("/welcome.html");
            break;
        }
        default:{
            m_targetfile_path.append(m_url);
            break;
            }
        }
    }
    else{
        m_targetfile_path.append(m_url);
    }
    
    if (stat(m_targetfile_path.data(), &m_file_stat) != 0)
    {
        cout << m_targetfile_path << " NO_RESOURCE" << endl;
        return NO_RESOURCE; //没有此资源
    }
    if(!(m_file_stat.st_mode & S_IROTH)){
        cout << m_targetfile_path << " FORBIDDEN_REQUEST" << endl;
        return FORBIDDEN_REQUEST;//没有可读权限
    }
    if(S_ISDIR(m_file_stat.st_mode)){
        cout << m_targetfile_path << " 此文件路径为目录" << endl;
        return BAD_REQUEST;//此文件路径为目录
    }
    int filefd = open(m_targetfile_path.data(), O_RDONLY);
    if(filefd==-1){
        cout << "file open failed" << endl;
    }
    else{
        
    }
    m_file_address = (char *)mmap(0, m_file_stat.st_size, PROT_READ, MAP_PRIVATE, filefd, 0);
    close(filefd);
    return FILE_REQUEST;
}

/*主状态机工作函数*/
http_conn::REQUEST_RESULT http_conn::process_read(){
    REQUEST_RESULT ret = NO_REQUEST;
    SLAVE_STATE line_state = SLAVE_STATE_LINEOK;
    char* text = nullptr;
    while (((m_master_state == MASTER_STATE_BODY)&&(line_state == SLAVE_STATE_LINEOK))||
            ((line_state = slave_parse_line())==SLAVE_STATE_LINEOK))
    {
        text = m_read_buff + m_startline_index;
        m_startline_index = m_check_index;
        switch (m_master_state)
        {
        case MASTER_STATE_REQUESTLINE:
            {
                ret = master_parse_line(text);
                if(ret == BAD_REQUEST){
                    return BAD_REQUEST;
                }
                break;
            }
        case MASTER_STATE_HEADER:
            {   
                ret = master_parse_header(text);
                if(ret == BAD_REQUEST){
                    return BAD_REQUEST;
                }
                else if(ret == GET_REQUEST){
                    return do_request();
                }
                break;
            }
        case MASTER_STATE_BODY:
            {   
                ret = master_parse_body(text);
                if(ret==GET_REQUEST){
                    return do_request();
                }
                line_state = SLAVE_STATE_LINEOPEN;
                break;
            }
        default:
            {
                cout << "INTERNAL_ERROR" << endl;
                return INTERNAL_ERROR;//返回服务器内部错误
            }
        }
    }
}

bool http_conn::read_once(){
    if(m_read_index >= READ_BUFFER_SIZE){
        return false;
    }
    int read_bytes = 0;
    /*循环读取，直到缓冲区内没有数据*/
    while(true){
        read_bytes = recv(m_sockfd,m_read_buff+m_read_index,READ_BUFFER_SIZE-m_read_index,0);
        if(read_bytes == -1 ){
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                break;
            }
            return false;
        }
        if(read_bytes == 0){
            return false;
        }
        m_read_index += read_bytes;
    }
    return true;
}

/*从状态机解析一行*/
http_conn::SLAVE_STATE http_conn::slave_parse_line(){
    char detect_char;
    for(; m_check_index < m_read_index; ++m_check_index){
        detect_char = m_read_buff[m_check_index];//获得要检测的字符
        if(detect_char == '\r'){
            if((m_check_index+1)>=m_read_index){
                return SLAVE_STATE_LINEOPEN;
            }
            if(m_read_buff[m_check_index+1] == '\n'){
                m_read_buff[m_check_index++] = '\0';
                m_read_buff[m_check_index++] = '\0';
                return SLAVE_STATE_LINEOK;
            }
            return SLAVE_STATE_LINEBAD;
        }
        else if(detect_char == '\n'){
            if(m_check_index > 1 && m_read_buff[m_check_index-1] == '\r'){
                m_read_buff[m_check_index-1] = '\0';
                m_read_buff[m_check_index++] = '\0';
                return SLAVE_STATE_LINEOK;
            }
            return SLAVE_STATE_LINEBAD;
        }
    }
    return SLAVE_STATE_LINEOPEN;
}

void http_conn::unmap(){
    if(m_file_address!=nullptr){
        munmap(m_file_address, m_file_stat.st_size);
        m_file_address = nullptr;
    }
}

bool http_conn::process_write(http_conn::REQUEST_RESULT ret){
    switch (ret)
    {
    case BAD_REQUEST:
        {add_status_line(400, error_400_title);
        add_header(error_500_form.size());
        if(!add_body(error_500_form.data())){
            return false;
        }
        break;}
    case FORBIDDEN_REQUEST:
        {add_status_line(403, error_403_title);
        add_header(error_403_form.size());
        if(!add_body(error_403_form.data())){
            return false;
        }
        break;}
    case NO_RESOURCE:
        {add_status_line(404, error_404_title);
        add_header(error_404_form.size());
        if(!add_body(error_404_form.data())){
            return false;
        }
        break;}
    case INTERNAL_ERROR:
        {add_status_line(500, error_500_title);
        add_header(error_500_form.size());
        if(!add_body(error_500_form.data())){
            return false;
        }
        break;}
    case FILE_REQUEST:
    {
        add_status_line(200, ok_200_title);
        if(m_file_stat.st_size!=0){
            add_header(m_file_stat.st_size);
            m_iv[0].iov_base = m_write_buff;
            m_iv[0].iov_len = m_write_index;
            m_iv[1].iov_base = m_file_address;
            m_iv[1].iov_len = m_file_stat.st_size;
            m_iv_count = 2;
            bytes_to_send = m_write_index + m_file_stat.st_size;
            return true;
        }
        else{
            /*没有文件要传输*/
            string readysend = "<html><body></body></html>";
            add_header(readysend.size());
            if(!add_body(readysend.data())){
                return false;
            }
        }
        break;
    }
    default:
        return false;
        break;
    }
    m_iv[0].iov_base = m_write_buff;
    m_iv[0].iov_len = m_write_index;
    m_iv_count = 1;
    return true;
}

bool http_conn::add_response(const char* format, ...){
    if(m_write_index>=WRITE_BUFFER_SIZE){
        return false;
    }
    va_list arg_list;//参数列表
    va_start(arg_list, format);
    int len = vsnprintf(m_write_buff + m_write_index, WRITE_BUFFER_SIZE - 1 - m_write_index, format, arg_list);
    if(len>=(WRITE_BUFFER_SIZE-1-m_write_index)){
        return false;//实际snprint只能写入WRITE_BUFFER_SIZE-1-m_write_index个字符
    }
    m_write_index += len;
    va_end(arg_list);
    return true;
}

bool http_conn::add_status_line(int status_code, const string &status_discrip){
    return add_response("%s %d %s\r\n", "HTTP/1.1", 200, status_discrip.data());
}

bool http_conn::add_header(int content_len){
    if(!add_response("Content-Length: %d\r\n",content_len)){
        return false;
    }
    if(!add_response("Connection: %s\r\n",(m_keep_alive==true?"keep-alive":"close"))){
        return false;
    }
    if(!add_blankline()){
        return false;
    }
    return true;
}

bool http_conn::add_body(const char* body){
    return add_response("%s", body);
}

bool http_conn::add_blankline(){
    return add_response("\r\n");
}

bool http_conn::write(){
    int ret = 0;
    if(bytes_to_send==0){
        //cout << m_sockfd << ":bytes_to_send==0,继续监听其输入" << endl;
        modfd(m_epollfd, m_sockfd, EPOLLIN);
        init();
        return true;
    }
    while(true){
        ret = writev(m_sockfd, m_iv, m_iv_count);
        //cout << m_sockfd << ":写入"<<ret<<"个byte" << endl;
        if(ret<0){
            if(errno==EAGAIN){
                //cout << "writev errno = EAGAIN,modfd(" << m_sockfd << ")" << endl;
                modfd(m_epollfd, m_sockfd, EPOLLOUT);
                return true;
            }
            unmap();
            return false;
        }
        bytes_have_send += ret;
        bytes_to_send -= ret;
        if(bytes_have_send>=m_iv[0].iov_len){
            m_iv[0].iov_len = 0;
            m_iv[1].iov_base = m_file_address + (bytes_have_send - m_write_index);
            m_iv[1].iov_len = bytes_to_send;
        }
        else{
            m_iv[0].iov_base = m_write_buff + bytes_have_send;
            m_iv[0].iov_len -= bytes_have_send;
        }

        if(bytes_to_send<=0){
            //cout << "bytes_to_send<=0,modfd(" << m_sockfd << ")" << endl;
            unmap();
            modfd(m_epollfd, m_sockfd, EPOLLIN);
            if(m_keep_alive){
                //cout << "m_keep_alive,init(" << m_sockfd << ")" << endl;
                init();
                return true;
            }
            else{
                //cout << m_sockfd<< ":no keep_alive" << endl;
                return false;
            }
        }
    }
}

void http_conn::linktimer(timerNode *timer){
    if(m_timer!=nullptr){
        delete m_timer;
        m_timer = nullptr;
    }
    m_timer = timer;
}

void http_conn::unlinktimer(){
    if(m_timer!=nullptr){
        m_timer = nullptr;
    }
}

void http_conn::init(){
    m_check_index = 0;
    m_startline_index = 0;
    m_read_index = 0;
    m_master_state = http_conn::MASTER_STATE_REQUESTLINE; //初始化时主状态机便在请求行上。;
    

    m_url = nullptr;
    m_method = GET;
    m_version = nullptr;
    
    m_content_length = 0;
    m_host = nullptr;
    m_keep_alive = false;//默认为短连接
    m_string = nullptr;
    memset(m_read_buff, '\0', READ_BUFFER_SIZE);
    memset(m_write_buff,'\0',WRITE_BUFFER_SIZE);

    m_write_index = 0;
    m_targetfile_path = "/home/allen/server_root";
    m_file_address = nullptr;
    m_iv_count = 2;
    bytes_to_send = 0;
    bytes_have_send = 0;
}

void http_conn::process_cgi(string &name, string &password){
    char *index = m_string;
    index = strchr(m_string, '=');
    if(index!=NULL){
        m_string = index + 1;
    }
    index = strchr(m_string, '&');
    if(index!=NULL){
        *index = '\0';
    }
    name.append(m_string);
    index++;
    index = strchr(m_string, '=');
    if(index!=NULL){
        m_string = index + 1;
    }
    password.append(m_string);
}

bool http_conn::user_is_valid(int state,const string& name,const string& password){
    if(state==0){
        /*当前为登陆*/
        if(!m_name_password.count(name)){
            return false;
        }
        if(m_name_password[name]!=password){
            return false;
        }
        return true;
    }
    else if(state==2){
        /*当前为注册*/
        if(m_name_password.count(name)){
            return false;/*用户名已经存在*/
        }
        else{
            m_name_password[name] = password;
            return true;/*注册成功*/
        }
    }
    return false;/*存在未知的状态*/
}