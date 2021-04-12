#include "timermanage.h"

timerNode::timerNode(http_conn *httpnode, int timeslot,bool isdelete):
    m_httpNode(httpnode),
    m_isdelete(isdelete){
    time_t cur = time(nullptr);
    expeire_time = cur + 3 * timeslot;
}

timerNode::~timerNode(){
    if(m_httpNode!=nullptr){
        delete m_httpNode;
        m_httpNode = nullptr;
    }
}

/*将此节点定义为要删除的节点*/
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

/*每次有新的socket连接，就需要新建一个定时器与其绑定，并将之前绑定的定时器删除*/
void timerManager::add_timer(http_conn *http, int timeslot){
    timerNode_ptr timer = new timerNode(http, timeslot);
    timer_heap.push(timer);
    http->linktimer(timer);
}

/*删除http绑定的定时器*/
void timerManager::del_timer(http_conn*http){

}

/*定时器管理器循环函数*/
void timerManager::handler(){

}