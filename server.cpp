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
    m_db = MyDB::getInstance(new ProduceSurl());//获取数据库操作类单例
    m_sqlpool = connection_pool::GetInstance();//获取数据库连接池类单例
    m_redis = Redis::getInstance();//获取redis连接类单例
    m_sqlpool->init("localhost", "root", "123456", "myserver", 3306, 4); //对数据库连接池初始化
    {
         
        //从数据库连接池中获取一个链接，对获取数据库的初始自增id；
        MYSQL *mysql = nullptr;
        connectionRAII mysqlcon(&mysql, m_sqlpool);
        m_db->initRedis(m_redis);
        if (!m_db->initID(mysql))
        {
            cout << "Error:First id get failed." << endl;
            exit(1);
        }
    }
}

server::~server(){
    if(users!=nullptr){
        for (int i = 0; i < MAX_FD;++i){
            users[i].http_close();
        }
        delete[] users;
    }
    if (m_epoolfd != -1)
    {
        close(m_epoolfd);
    }
    if(m_listenfd){
        close(m_listenfd);
    }
    if(m_threadpool!=nullptr){
        delete m_threadpool;
    }
    
    if(m_pipefd[0]!=0){
        close(m_pipefd[0]);
    }
}

/*server初始化函数*/
void server::init(int port, int threadnum){
    m_port = port;
    m_thread_number = threadnum;
    
    threadpool_init(m_thread_number, MAX_FD, m_sqlpool);
}

/*线程池初始化函数*/
void server::threadpool_init(int threadnum, int max_request_number,connection_pool* connPool){
    /*单例模式获取线程池指针*/
    m_threadpool = threadpool<http_conn>::getInstance(threadnum, max_request_number,connPool);
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
    http_conn::addfd(m_epoolfd, m_listenfd);/*listenFD最好为LT模式*/

    /*定时相关*/
    ret = socketpair(PF_UNIX,SOCK_STREAM,0,m_pipefd);/*初始化信号管道*/
    assert(ret != -1);
    http_conn::setnoblock(m_pipefd[1]);
    http_conn::addfd(m_epoolfd, m_pipefd[0]);
    
    /*SIGPIPE信号：
    客户端异常关闭socket后，server收到FIN信号，如果此时server向client写数据，第一次会受到RST
    第二次会受到SIGPIPE，其默认是关闭当前进程，所以需要忽略*/
    m_timerManger.addsig(SIGPIPE, SIG_IGN);/*SIG_IGN表示忽略SIGPIPE信号*/
    m_timerManger.addsig(SIGALRM, m_timerManger.sig_hander); /*添加定时信号*/
    m_timerManger.addsig(SIGTERM, m_timerManger.sig_hander);/*终止进程信号*/
    alarm(m_timerManger.get_timerlot()); /*开启定时*/
    timerManager::timer_pipefd = m_pipefd;
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
                //cout << "server listen a new connect:"<< connfd << endl;
                users[connfd].init(connfd, client_address,m_db);
                //m_timerManger.add_timer(&users[connfd]);/*向时间堆中添加定时器，并于http绑定*/
            }
            //EPOLLRDHUP:客户端关闭，发送FIN
            //EPOLLHUP:服务器段socket出错
            //EPOLLERR:读写出错
            else if(m_events[i].events & (EPOLLRDHUP|EPOLLHUP|EPOLLERR)){
                //cout << "m_events[" << sockfd << "].events & (EPOLLRDHUP|EPOLLHUP|EPOLLERR)" << endl;
                users[sockfd].http_close(); //关闭客户端连接
            }
            /*接收到信号*/
            else if (sockfd == m_pipefd[0] && (m_events[i].events & EPOLLIN)){
                dealwith_sig(timeout, isStop);
            }
            /*接收到数据*/
            else if (m_events[i].events & (EPOLLIN))
            {
                //cout << "m_events[" << sockfd << "].events & EPOLLIN" << endl;
                if (users[sockfd].read_once())
                {
                    //cout << "users[" << sockfd << "].read_once" << endl;
                    m_threadpool->append(&users[sockfd]);
                    m_timerManger.updata(users[sockfd].get_timer()); /*更新定时器*/
                }
                else{
                    users[sockfd].http_close();
                }
            }
            /*有数据要写*/
            else if(m_events[i].events & EPOLLOUT){
                //cout << "m_events[" << sockfd << "].events & EPOLLOUT" << endl;
                if(users[sockfd].write()){
                    //cout << "m_events[" << sockfd << "] write successful" << endl;
                    m_timerManger.updata(users[sockfd].get_timer()); /*更新定时器*/
                }
                else{
                    //cout << sockfd << ": close" << endl;
                    users[sockfd].http_close(); //关闭客户端连接
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
    int ret = recv(m_pipefd[0], sig, sizeof(sig),0);
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