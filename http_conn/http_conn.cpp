#include "http_conn.h"

/*将文件描述符设置成非堵塞*/
void setnoblock(int fd){
    int old_option = fcntl(fd,F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd,F_SETFL,new_option);
}

/*向epollfd中添加监听的socket*/
void addfd(int epollfd,int fd){
    epoll_event event;
    event.data.fd = fd;
    //EPOLLIN:有新联接或有数据到来，EPOLLRDHUP:对方是否关闭
    //EPOLLONESHOT:信号到达之后，除非重新调用eppol_ctl，否则不会再次唤醒线程，
    //保证一个socket被一个线程处理
    event.events |= EPOLLIN | EPOLLRDHUP | EPOLLONESHOT;
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

int http_conn::m_epollfd = -1;//静态变量初始化，
int http_conn::m_user_count = 0;//静态变量初始化，

void http_conn::process(){
    REQUEST_RESULT result = process_read();

}

http_conn::REQUEST_RESULT http_conn::process_read(){
    
}

void http_conn::init(int sockfd,const sockaddr_in& addr){
    m_sockfd = sockfd;
    m_address = addr;
    m_check_index = 0;
    m_startline_index = 0;
    m_read_index = 0;
    
    addfd(m_epollfd,sockfd);
    m_user_count++;

    m_curstate_master = http_conn::MASTER_STATE_REQUESTLINE;//初始化时主状态机便在请求行上。

    memset(m_read_buff,'/0',READ_BUFFER_SIZE);
    memset(m_write_buff,'/0',WRITE_BUFFER_SIZE);
}