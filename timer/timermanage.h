#ifndef TIMERMANAGE_H_
#define TIMERMANAGE_H_
#include <queue>
#include <unistd.h>
#include <sys/time.h>
#include <signal.h>
#include "../http_conn/http_conn.h"
class http_conn;

/*定时器节点类*/
class timerNode
{
private:
    http_conn *m_httpNode;/*指向对应的http工作类*/
    time_t expeire_time;/*到期时间*/
    bool m_isdelete;/*是否是一个要删除的节点*/

public:
    timerNode() :m_httpNode(nullptr),expeire_time(0){}
    timerNode(http_conn *httpnode, int timeslot = 5,bool isdelete = false);
    void setDelete();
    bool isDelete() const { return m_isdelete; }
    bool isExpeire();
    void updata(int timeslot = 5);
    http_conn *getHttp() const { return m_httpNode; }
    ~timerNode();
    time_t getTimerExpeir() const { return expeire_time; }
    
};
/*小根堆自定义比较函数*/
struct timerCmp{
    bool operator()(timerNode* &a,timerNode* &b)const{
        return a->getTimerExpeir() > b->getTimerExpeir();
    }
};

/*定时器管理类*/
class timerManager
{
private:
    typedef timerNode* timerNode_ptr;
    std::priority_queue<timerNode_ptr, std::vector<timerNode_ptr>, timerCmp> timer_heap;
    int m_timerslot;/*每隔3*m_timerslot进行一次心跳执行函数*/

public:
    static int *timer_pipefd;/*信号管道写端*/
    void add_timer(http_conn *http, int timeslot = 5);
    void del_timer(http_conn *http);
    void handler();
    timerManager() { m_timerslot = 15; }
    timerManager(int timerslot):m_timerslot(timerslot*3){}
    ~timerManager();
    int get_timerlot() const { return m_timerslot; }
    static void sig_hander(int sig);/*信号的响应函数*/
    void addsig(int sig,void(sig_hander)(int)); /*往信号集里添加信号*/
    void updata(timerNode_ptr timer);/*更新定时器节点，在堆中找到这个节点，并将其
                                        到期时间更改，再将其重新押入堆中*/
};

#endif