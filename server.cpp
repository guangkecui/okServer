#include "server.h"
#include <cassert>
using std::cout;
using std::endl;

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
server::server()
{
    m_port = 0;
    m_epoolfd = 0;
    m_thread_number = 0;
    users = new http_conn[MAX_FD];
}

server::~server(){
    close(m_epoolfd);
    close(m_listenfd);
    delete m_threadpool;
    delete[] users;
}

/*server初始化函数*/
void server::init(int port, int threadnum){
    m_port = port;
    m_thread_number = threadnum;
}

/*线程池初始化函数*/
void server::threadpool_init(int threadnum, int max_request_number){
    /*单例模式获取线程池指针*/
    m_threadpool = threadpool<http_conn>::getInstance(threadnum, max_request_number);
}

/*开始监听*/
void server::start_listen(){
    m_listenfd = listen(PF_INET, SOCK_STREAM);
    assert(m_listenfd >= 0);

    int ret = 0;
    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(m_port);

    /*关闭listenfd的TIME——WAIT,防止服务器断电之后可以立马重复使用listenfd*/
    int mReuseadress = 1;
    setsockopt(m_listenfd, SOL_SOCKET, SO_REUSEADDR, &mReuseadress, sizeof(mReuseadress));
    ret = bind(m_listenfd, (struct sockaddr *)&address, sizeof(address));
    assert(ret >= 0);

    
    m_epoolfd = epoll_create(5);
    assert(m_epoolfd != -1);
    http_conn::m_epollfd = m_epoolfd;
    addfd(m_epoolfd, m_listenfd);
}
/*server主线程时间循环函数*/
void server::event_loop(){
    bool isStop = false;
    epoll_event events[MAX_EVENT_NUMBER];//创建epoll_event事件数组
    while(!isStop){
        int number = epoll_wait(m_epoolfd, events, MAX_EVENT_NUMBER, -1);
        if(number<1||errno!=EINTR){
            cout << "epoll_wait failed!" << endl;
            break;
        }
        for (int i = 0; i < number; ++i){
            int sockfd = events[i].data.fd;
            if(sockfd == m_listenfd){
                struct sockaddr_in client_address;
                socklen_t client_address_length = sizeof(client_address);
                int connfd = accept(m_listenfd, (struct sockaddr *)&client_address, &client_address_length);
                if(connfd < 0){
                    cout << "connfd < 0,errno:" << errno << endl;
                    continue;
                }
                if(http_conn::m_user_count > MAX_FD){
                    cout << "server internal busy!" << endl;
                    continue;
                }
                users[connfd].init(connfd, client_address);
            }
            //EPOLLRDHUP:客户端关闭，发送FIN
            //EPOLLHUP:读写端关闭
            //EPOLLERR:读写出错
            else if(events[i].events & (EPOLLRDHUP|EPOLLHUP|EPOLLERR)){
                
            }
        }
    }
}

