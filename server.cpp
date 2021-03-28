#include "server.h"
#include <cassert>
server::server(){
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

    int mReuseadress = 1;
    
}
