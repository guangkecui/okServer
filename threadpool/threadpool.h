#ifndef THREADPOOL_H_
#define THREADPOOL_H_

#include<list>
#include<exception>
#include<pthread.h>
#include<iostream>
#include"locker.h"

#define M_THREAD_MAXNUMBER 15/*限制的线程池内线程的最大数量*/
template < typename T>
class threadpool
{
private:
    int m_thread_number;/*线程池中线程的数量*/
    pthread_t* m_threads;/*描述线程池的数组*/
    int m_max_requests; /*请求队列中允许的最大请求数*/
    std::list<T*> m_workqueue;/*任务的请求队列*/
    locker m_queuelocker;/*互斥锁，访问队列时用到*/
    sem m_queuesem; /*信号量，队列中是否有任务需要处理*/
    bool m_stop;/*是否结束线程标志位*/
    /*工作线程的任务执行函数，工作线程由此开始运行*/
    static void* worker(void* arg);
    /*线程池运行函数，被工作线程调用*/
    void run();
    /*构造函数设置成私有函数，防止外部初始化*/
    threadpool(int thread_number,int max_requests);
public:
    /*单例模式获得线程池对象*/
    static threadpool<T>* getInstance(int thread_number,int max_requests);
    ~threadpool();
    /*往任务请求队列中插入任务*/
    bool append(T* request);
    
};

template < typename T>
threadpool<T>::threadpool(int thread_number,int max_requests):
                        m_thread_number(thread_number),
                        m_max_requests(max_requests),
                        m_stop(false),m_threads(nullptr)
{
    std::cout<<"线程池构造"<<std::endl;
    if(thread_number<=0||max_requests<=0){
        throw std::exception();
    }
    m_threads = new pthread_t[m_thread_number];/*申请线程数组*/
    if(!m_threads){
        throw std::exception();
    }

    /*创建m_thread_number个线程，并将所有的线程设置成分离线程*/
    for(int i = 0;i < m_thread_number; ++i){
        std::cout<<"creat the "<<i<<"th thread."<<std::endl;
        if(pthread_create(m_threads+i,NULL,worker,this)!=0){
            delete[] m_threads;
            throw std::exception();
        }
        //将线程设置成分离模式
        if(pthread_detach(m_threads[i])!=0){
            delete[] m_threads;
            throw std::exception();
        }
    }
}

template < typename T>
threadpool<T>::~threadpool()
{
    delete [] m_threads;
    m_stop = true;
    
    std::cout<<"线程池析构"<<std::endl;
}

template<typename T>
bool threadpool<T>::append(T* request){
    /*首先对工作队列加锁，防止其他线程访问工作队列*/
    m_queuelocker.lock();
    if(m_workqueue.size()>m_max_requests){
        m_queuelocker.unlock();
        return false;
    }
    m_workqueue.push_back(request);
    m_queuelocker.unlock();
    m_queuesem.post();/*信号量+1，阻塞在此信号量上的线程被唤醒*/
    return true;
}

template < typename T>
void threadpool<T>::run(void){
    std::cout<<"working pthread is in run_function."<<pthread_self()<<std::endl;
    while(!m_stop){
        m_queuesem.wait();/*等待信号大于0，即等待工作队列不为空*/
        m_queuelocker.lock();               /*----------------------*/
        if(m_workqueue.empty()){            /*------临界区-----------*/
            m_queuelocker.unlock();         /*------临界区-----------*/
            continue;                       /*------临界区-----------*/
        }                                   /*------临界区-----------*/
        T* request = m_workqueue.front();   /*------临界区-----------*/
        m_workqueue.pop_front();            /*------临界区-----------*/
        m_queuelocker.unlock();             /*----------------------*/
        if(!request){
            continue;
        }
        request->process();
    }
}

template<typename T>
void* threadpool<T>::worker(void* arg){
    std::cout<<"working pthread is in work_function."<<pthread_self()<<std::endl;
    threadpool* pool = (threadpool*)arg;
    pool->run();
    return pool;
}

template<typename T>
threadpool<T>* threadpool<T>::getInstance(int thread_number, int max_requests){
    /*静态局部变量，返回构造单例*/
    static  threadpool<T> m_threadpool_local( thread_number, max_requests);
    return &m_threadpool_local;
}

#endif 