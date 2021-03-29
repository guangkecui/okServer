#include "server.h"

using std::cout;
using std::endl;

//template<class T>
//threadpool<T>* threadpool<T>::m_threadpool;
int main(void){
    server m_server;
    m_server.init(6969, 4);
    m_server.event_loop();

    return 0;
}
