#include "timermanage.h"
#include <assert.h>
#include <iostream>
#include <stack>
using std::cout;
using std::endl;
using std::stack;

timerNode::timerNode(http_conn *httpnode, int timeslot, bool isdelete) : m_httpNode(httpnode),
                                                                         m_isdelete(isdelete)
{
    time_t cur = time(nullptr);
    expeire_time = cur + 3 * timeslot;
}

timerNode::~timerNode(){
    m_httpNode = nullptr;
}

/*将此节点定义为要删除的节点
两种情况会设置delete
1:定期循环函数到达，询问此节点是否到期，关闭非活动连接
2：客户端主动关闭连接，服务器将此节点设置为到期*/
void timerNode::setDelete(){
    m_isdelete = true;
}

bool timerNode::isExpeire(){
    time_t cur = time(nullptr);
    if(cur >= expeire_time){
        setDelete();
        return true;
    }
    else{
        return false;
    }
}

/*更新定时器，每次socket有新的数据到来，或者刚发送完数据，就需要
更新定时器*/
void timerNode::updata(int timeslot){
    time_t cur = time(nullptr);
    expeire_time = cur + 3 * timeslot;
}

int * timerManager::timer_pipefd = nullptr;/*静态变量初始化*/

timerManager::~timerManager()
{
    while(!timer_heap.empty()){
        timerNode_ptr curtimer = timer_heap.top();
        curtimer->getHttp()->unlinktimer();
        delete curtimer;
        curtimer = nullptr;
    }
}

/*每次有新的socket连接，就需要新建一个定时器与其绑定，并将之前绑定的定时器删除*/
void timerManager::add_timer(http_conn *http, int timeslot){
    timerNode_ptr timer = new timerNode(http, timeslot);
    timer_heap.push(timer);
    http->linktimer(timer);
}

/*删除http绑定的定时器*/
void timerManager::del_timer(http_conn*http){
    http->get_timer()->setDelete();/*将定时器节点设置成关闭*/
}

/*定时器管理器循环函数*/
void timerManager::handler(){
    while (!timer_heap.empty())
    {
        timerNode_ptr curtimer = timer_heap.top();
        if(curtimer->isDelete()){
            timer_heap.pop();
            curtimer->getHttp()->unlinktimer();
            delete curtimer;
            curtimer = nullptr;
            
        }
        else if(curtimer->isExpeire()){
            curtimer->getHttp()->http_close();
            timer_heap.pop();
            delete curtimer;
            curtimer = nullptr;
        }
        else{
            break;
        }
    }
    alarm(m_timerslot);/*重新定时*/
}

void timerManager::sig_hander(int sig){
    /*保留原来的errno*/
    int old_errno = errno;
    int msg = sig;/*要写的信息就是信号值*/
    send(timer_pipefd[1], (char *)&msg, 1, 0);/*将信号写入管道,通知主线程epoll接受，目的是统一事件源*/
    errno = old_errno;
}

void timerManager::addsig(int sig,void(sig_hander)(int)){
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = sig_hander;
    sigfillset(&sa.sa_mask);
    assert(sigaction(sig, &sa, nullptr) != -1);
}

void timerManager::updata(timerNode_ptr timer){
    stack<timerNode_ptr> timer_stack;
    while (!timer_heap.empty())
    {
        timerNode_ptr curtimer = timer_heap.top();
        timer_heap.pop();
        if(curtimer==timer){
            timer_stack.push(curtimer);
            curtimer->updata();
            break;
        }
        else if(curtimer->isDelete()){
            curtimer->getHttp()->unlinktimer();
            delete curtimer;
            curtimer = nullptr;
            
        }
        else if(curtimer->isExpeire()){
            curtimer->getHttp()->http_close();
            delete curtimer;
            curtimer = nullptr;
        }
        else{
            timer_stack.push(curtimer);
        }
        
    }
    while(!timer_stack.empty()){
        timerNode_ptr toptimer = timer_stack.top();
        timer_stack.pop();
        timer_heap.push(toptimer);
    }
}