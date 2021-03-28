#include "http_conn.h"
#include <iostream>
using std::cout;
using std::endl;

int http_conn::m_epollfd = -1;//静态变量初始化，
int http_conn::m_user_count = 0;//静态变量初始化，
/*将文件描述符设置成非堵塞*/
void setnoblock(int fd){
    int old_option = fcntl(fd,F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd,F_SETFL,new_option);
}

/*向epollfd中添加监听的socket*/
void addfd(int epollfd,int fd,int isShot = false){
    epoll_event event;
    event.data.fd = fd;
    //EPOLLIN:有新联接或有数据到来，EPOLLRDHUP:对方是否关闭
    //EPOLLONESHOT:信号到达之后，除非重新调用eppol_ctl，否则不会再次唤醒线程，
    //保证一个socket被一个线程处理
    event.events |= EPOLLIN | EPOLLRDHUP ;
    if(isShot){
        event.events |= EPOLLONESHOT;
    }
    epoll_ctl(epollfd,EPOLL_CTL_ADD,fd,&event);
    setnoblock(fd);
}

/*从内核事件表中删除fd*/
void removefd(int epollfd,int fd){
    epoll_ctl(epollfd,EPOLL_CTL_DEL,fd,0);
    close(fd);
}

/*将事件重置为ONESHOT*/
void modfd(int epollfd,int fd,int old_option){
    epoll_event event;
    event.data.fd = fd;
    event.events = old_option | EPOLLONESHOT | EPOLLRDHUP;
    epoll_ctl(epollfd,EPOLL_CTL_MOD,fd,&event);
}
void http_conn::init(int sockfd,const sockaddr_in& addr){
    m_sockfd = sockfd;
    m_address = addr;
    m_check_index = 0;
    m_startline_index = 0;
    m_read_index = 0;
    m_master_state = 0;
    addfd(m_epollfd, sockfd);
    m_curstate_master = http_conn::MASTER_STATE_REQUESTLINE; //初始化时主状态机便在请求行上。
    m_user_count++;

    m_url = nullptr;
    m_method = GET;
    m_version = nullptr;
    
    m_content_length = 0;
    m_host = nullptr;
    m_keep_alive = false;
    m_string = nullptr;
    memset(m_read_buff, '/0', READ_BUFFER_SIZE);
    memset(m_write_buff,'/0',WRITE_BUFFER_SIZE);
}

void http_conn::process(){
    REQUEST_RESULT result = process_read();
    if(result == NO_REQUEST){
        modfd(m_epollfd,m_sockfd,EPOLLIN);
        return;
    }
}

http_conn::REQUEST_RESULT http_conn::master_parse_line(char* text){
    m_url = strpbrk(text, " \t"); //在text中找到第一个空格的位置
    if(!m_url){
        return BAD_REQUEST;
    }
    *m_url++ = '\0';//加入结束符

    char *method = text;//text指向GET或POST
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
    *m_version++ = '\0';
    m_version += strspn(m_version, " \t");//将mervison移动到不是\t的位置

    if(strcasecmp(m_version,"HTTP/1.1")!=0){
        return BAD_REQUEST;
    }

    if(strncasecmp(m_url,"http://",7)){
        //去掉http：//
        m_url += 7;
        m_url = strchr(m_url, '/'); //将url指向第一次出现‘/’的地方;
    }
    if(!m_url||m_url[0]!='/'){
        return BAD_REQUEST;
    }
    m_master_state = MASTER_STATE_HEADER;
    return NO_REQUEST;
}

http_conn::REQUEST_RESULT http_conn::master_parse_header(char* text){
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
    }
    /*处理content-length*/
    else if(strncasecmp(text,"Content-Length:",15)){
        text += 15;
        text += strspn(text, " \t");
        m_content_length = atoi(text);
    }
    /*处理HOST*/
    else if(strncasecmp(text,"Host:",4)){
        text += 5;
        text += strspn(text, " \t");
        m_host = text;
    }
    /*其余暂时不处理*/
    else{
        std::cout << "unknow header:" << text << std::endl;
    }
    return NO_REQUEST;
}

http_conn::REQUEST_RESULT http_conn::master_parse_body(char* text){
    if((m_check_index+m_content_length)<=m_read_index){//请求体被完整读入
        text[m_content_length] = '\0';
        m_string = text;
        return GET_REQUEST;
    }
    return NO_REQUEST;
}

http_conn::REQUEST_RESULT http_conn::do_request(){
    
}

/*主状态机工作函数*/
http_conn::REQUEST_RESULT http_conn::process_read(){
    REQUEST_RESULT ret = NO_REQUEST;
    SLAVE_STATE line_state = SLAVE_STATE_LINEOK;
    char* text = nullptr;
    while ((m_master_state == MASTER_STATE_BODY)&&(line_state == SLAVE_STATE_LINEOK)||
            (line_state = slave_parse_line())==SLAVE_STATE_LINEOK)
    {
        text = m_read_buff + m_startline_index;
        m_startline_index = m_check_index;
        std::cout << "http got 1 http line:" << text << std::endl;
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
            if(errno == EAGAIN || errno == EWOULDBLOCK){
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
            if((m_check_index+1)==m_read_index){
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
            if(m_check_index>0 && m_read_buff[m_check_index-1] == '\r'){
                m_read_buff[m_check_index-1] = '\0';
                m_read_buff[m_check_index++] = '\0';
                return SLAVE_STATE_LINEOK;
            }
            return SLAVE_STATE_LINEBAD;
        }
    }
    return SLAVE_STATE_LINEOPEN;
}













