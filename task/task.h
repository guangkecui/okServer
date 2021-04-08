#ifndef TASK_H_
#define TASK_H_
#include<iostream>
#include<pthread.h>
using namespace std;
class task
{
private:
    /* data */
public:
    task(/* args */){
        
    }
    ~task();
    void process(void){
        cout<<"task is start working;--"<<pthread_self()<<endl;
        for(int i=0;i<100000;++i){
        }
        cout<<"task is end working;--"<<pthread_self()<<endl;
    }
};


#endif