#ifndef SERVER_H_
#define SERVER_H_
#include "./http_conn/http_conn.h"
#include "./threadpool/threadpool.h"

const int MAX_FD = 65535;//最大文件描述符
const int MAX_EVENT_NUMBER = 10000; //最大事件数;

class server
{
private:
    int m_port;//服务器监听的端口
    int m_epoolfd;//epool描述符
    http_conn *users;//任务数组
    threadpool<http_conn> *m_threadpool; //线程池
    int m_listenfd;//监听的socket描述符

public:
    server();
    ~server();
};



#endif