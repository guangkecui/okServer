#ifndef SERVER_H_
#define SERVER_H_
#include "./http_conn/http_conn.h"
#include "./threadpool/threadpool.h"
#include "./timer/timermanage.h"
#include "./redis/myredis.h"
#define READ_EVENT 0;
#define WRITE_EVENT 1;
const int MAX_FD = 65535;           //最大文件描述符
const int MAX_EVENT_NUMBER = 10000; //最大事件数;

class server
{
private:
    int m_port;//服务器监听的端口
    int m_epoolfd;//epool描述符
    int m_pipefd[2];//信号管道
    http_conn *users;                    //任务数组
    threadpool<http_conn> *m_threadpool; //线程池
    int m_listenfd;//监听的socket描述符
    int m_thread_number;//线程池中线程个数
    timerManager m_timerManger;//定时器管理器
    MyDB *m_db;//数据库操作类
    connection_pool *m_sqlpool;//数据库连接池指针
    Redis *m_redis;//redis缓存

public:
    server();
    ~server();
    void init(int port, int threadnum);
    void threadpool_init(int threadnum, int max_request_number,connection_pool* connPool);
    void start_listen();
    bool dealwith_sig(bool & timeout, bool & isServerStop);/*处理epoll受到的信号*/
    int event_loop();
};

#endif