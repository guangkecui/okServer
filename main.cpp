#include"threadpool.h"
#include"task.h"

using namespace std;

//template<class T>
//threadpool<T>* threadpool<T>::m_threadpool;
int main(void){
    cout<<"第一次调用之前"<<endl;
    threadpool<task>* pool = threadpool<task>::getInstance(4,8);
    cout<<"第一次调用之后"<<endl;
    cout<<"第二次调用之前"<<endl;
    threadpool<task>* pool2 = threadpool<task>::getInstance(4,8);
    cout<<"第二次调用之后"<<endl;
    for(int i = 0;i<8;++i){
        task* a = new task();
        pool->append(a);
    }
    while(getchar()!='q');
    return 0;
}