#ifndef TIMERMANAGE_H_
#define TIMERMANAGE_H_
#include <queue>
#include <unistd.h>
#include <sys/time.h>
#include "http_conn/http_conn.h"

class http_conn;

/*定时器节点类*/
class timerNode
{
private:
    http_conn *m_httpNode;/*指向对应的http工作类*/
    time_t expeire_time;/*到期时间*/
public:
    timerNode() :m_httpNode(nullptr),expeire_time(0){}
    timerNode(http_conn *httpnode, int timeslot = 5);
    void updata();
    ~timerNode();
};


#endif