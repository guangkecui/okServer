#include "timermanage.h"

timerNode::timerNode(http_conn *httpnode, int timeslot):m_httpNode(httpnode){
    time_t cur = time(nullptr);
    expeire_time = cur + 3 * timeslot;
}

timerNode::~timerNode(){
    if(m_httpNode!=nullptr){
        delete m_httpNode;
        m_httpNode = nullptr;
    }
}

void timerNode::updata(){
    
}
