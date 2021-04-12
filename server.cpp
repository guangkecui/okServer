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

    /*定时相关*/
    /*SIGPIPE信号：
    客户端异常关闭socket后，server收到FIN信号，如果此时server向client写数据，第一次会受到RST
    第二次会受到SIGPIPE，其默认是关闭当前进程，所以需要忽略*/
    m_timerManger.addsig(SIGPIPE, SIG_IGN);/*SIG_IGN表示忽略SIGPIPE信号*/
    m_timerManger.addsig(SIGALRM, m_timerManger.sig_hander); /*添加定时信号*/
    m_timerManger.addsig(SIGTERM, m_timerManger.sig_hander);/*终止进程信号*/
    alarm(m_timerManger.get_timerlot()); /*开启定时*/
}
/*server主线程时间循环函数*/
int server::event_loop(){
    bool isStop = false;
    bool timeout = false;/*定时器是否到时*/
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
                m_timerManger.add_timer(&users[connfd]);/*向时间堆中添加定时器，并于http绑定*/
            }
            //EPOLLRDHUP:客户端关闭，发送FIN
            //EPOLLHUP:服务器段socket出错
            //EPOLLERR:读写出错
            else if(m_events[i].events & (EPOLLRDHUP|EPOLLHUP|EPOLLERR)){
                cout << "m_events[" << sockfd << "].events & (EPOLLRDHUP|EPOLLHUP|EPOLLERR)" << endl;
                users[sockfd].http_close(); //关闭客户端连接
            }
            /*接收到信号*/
            else if (sockfd == timerManager::timer_pipefd[0] && (m_events[i].events & EPOLLIN)){
                dealwith_sig(timeout, isStop);
            }
            /*接收到数据*/
            else if (m_events[i].events & (EPOLLIN))
            {
                cout << "users[" << sockfd << "].read_once()" << endl;
                if (users[sockfd].read_once())
                {
                    m_threadpool->append(&users[sockfd]);
                    m_timerManger.updata(users[sockfd].get_timer()); /*更新定时器*/
                }
                else{
                    users[sockfd].http_close();
                }
            }
            /*有数据要写*/
            else if(m_events[i].events & EPOLLOUT){
                cout << "users[" << sockfd << "].write" << endl;
                if(users[sockfd].write()){
                    cout << "write successful" << endl;
                }
                else{

                    cout << "close keep alive" << endl;
                }
            }
            /*忽略其他事件*/
            else{

            }
            if(timeout){
                m_timerManger.handler();
                timeout = false;
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

bool server::dealwith_sig(bool & timeout, bool & isServerStop){
    char sig[1024];
    int res = 0;
    int ret = recv(timerManager::timer_pipefd[0], sig, sizeof(sig),0);
    if(ret==-1){
        cout << "server receive error signal" << endl;
        return false;
    }
    else if(ret==0){
        cout << "server receive 0 signal" << endl;
        return false;
    }
    else{
        for (int i = 0; i < ret; ++i){
            switch (sig[i])
            {
            case SIGTERM:{
                isServerStop = true;
                break;
            }
            case SIGALRM:{
                timeout = true;
                break;
            }
            default:
                break;
            }
        }
    }
    return true;
}