#include "server.h"
#include <cassert>
using std::cout;
using std::endl;


server::server()
{
    m_port = 0;
    m_epoolfd = -1;
    m_thread_number = 0;
    m_listenfd = -1;
    users = new http_conn[MAX_FD];
}

server::~server(){
    if(m_epoolfd!=-1){
        close(m_epoolfd);
    }
    if(m_listenfd){
        close(m_listenfd);
    }
    if(m_threadpool!=nullptr){
        delete m_threadpool;
    }
    if(users!=nullptr){
        delete[] users;
    }
    
}

/*server初始化函数*/
void server::init(int port, int threadnum){
    m_port = port;
    m_thread_number = threadnum;
    threadpool_init(m_thread_number, MAX_FD);
}

/*线程池初始化函数*/
void server::threadpool_init(int threadnum, int max_request_number){
    /*单例模式获取线程池指针*/
    m_threadpool = threadpool<http_conn>::getInstance(threadnum, max_request_number);
}

/*开始监听*/
void server::start_listen(){
    m_listenfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(m_listenfd >= 0);
    cout << "listenfd = " << m_listenfd << endl;
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

    ret = listen(m_listenfd,5);
    assert(ret >= 0);

    m_epoolfd = epoll_create(5);
    assert(m_epoolfd != -1);
    cout << "m_epoolfd = " << m_epoolfd << endl;
    http_conn::m_epollfd = m_epoolfd;
    http_conn::addfd(m_epoolfd, m_listenfd);
}
/*server主线程时间循环函数*/
int server::event_loop(){
    bool isStop = false;
    epoll_event m_events[MAX_EVENT_NUMBER]; //创建epoll_event事件数组
    start_listen();
    while(!isStop){
        int number = epoll_wait(m_epoolfd, m_events, MAX_EVENT_NUMBER, -1);
        if(number<1 && errno!=EINTR){
            cout << "errno :" << errno << endl;
            cout << "epoll_wait failed!" << endl;
            break;
        }
        for (int i = 0; i < number; ++i){
            int sockfd = m_events[i].data.fd;
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
                cout << "server listen a new connect." << endl;
                users[connfd].init(connfd, client_address);
            }
            //EPOLLRDHUP:客户端关闭，发送FIN
            //EPOLLHUP:服务器段socket出错
            //EPOLLERR:读写出错
            else if(m_events[i].events & (EPOLLRDHUP|EPOLLHUP|EPOLLERR)){
                cout << "m_events[" << sockfd << "].events & (EPOLLRDHUP|EPOLLHUP|EPOLLERR)" << endl;
                users[sockfd].http_close(); //关闭客户端连接
            }
            /*接收到数据*/
            else if(m_events[i].events & (EPOLLIN)){
                cout << "users[" << sockfd << "].read_once()" << endl;
                if(users[sockfd].read_once()){
                    m_threadpool->append(&users[sockfd]);
                }
                else{
                    users[sockfd].http_close();
                }
            }
            /*有数据要写*/
            else if(m_events[i].events & EPOLLOUT){
                cout << "users[" << sockfd << "].write" << endl;
                if(users[sockfd].write()){
                    cout << "write success,keep alive." << endl;
                }
                else{
                    cout << "close keep alive" << endl;
                }
            }
            /*忽略其他事件*/
            else{

            }
        }
    }
    if(m_epoolfd!=-1){
        close(m_epoolfd);
        m_epoolfd = -1;
    }
    if(m_listenfd!=-1){
        close(m_listenfd);
        m_listenfd = -1;
    }
    if(users!=nullptr){
        delete[] users;
        users = nullptr;
    }
    if(m_threadpool!=nullptr){
        delete m_threadpool;
        m_threadpool = nullptr;
    }
    
    return 0;
}
