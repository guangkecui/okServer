#ifndef LOCKER_H_
#define LOCKER_H_

#include<exception>
#include<pthread.h>
#include<semaphore.h>

/*封装信号量的类*/
class sem
{
private:
    sem_t m_sem;
public:
    /*创建并初始化信号量*/
    sem(){
        if(sem_init(&m_sem,0,0)!=0){
            throw std::exception();
        }
    }
    /*信号量析构函数*/
    ~sem(){
        sem_destroy(&m_sem);
    }
    /*等待信号量*/
    bool wait(){
        return sem_wait(&m_sem)==0;
    }
    /*增加信号量*/
    bool post(){
        return sem_post(&m_sem)==0;
    }
};

/*封装互斥锁的类*/
class locker
{
private:
    pthread_mutex_t m_mutex;
public:
    locker(){
        if(pthread_mutex_init(&m_mutex,NULL)!=0){
            throw std::exception();
        }
    }
    ~locker(){
        pthread_mutex_destroy(&m_mutex);
    }

    bool lock(){
        return pthread_mutex_lock(&m_mutex)==0;
    }

    bool unlock(){
        return pthread_mutex_unlock(&m_mutex)==0;
    }
};

/*封装条件变量的类*/
class cond
{
private:
    pthread_cond_t m_cond;
    pthread_mutex_t m_mutex;
public:
    cond(){
        if(pthread_mutex_init(&m_mutex,NULL)!=0){
            throw std::exception();
        }
        if(pthread_cond_init(&m_cond,NULL)!=0){
            pthread_mutex_destroy(&m_mutex);
            throw std::exception();
        }
    }
    ~cond(){
        pthread_mutex_destroy(&m_mutex);
        pthread_cond_destroy(&m_cond);
    }

    bool wait(){
        int ret = 0;
        pthread_mutex_lock(&m_mutex);
        ret = pthread_cond_wait(&m_cond,&m_mutex);
        pthread_mutex_unlock(&m_mutex);
        return ret==0;
    }

    bool signal(){
        return pthread_cond_signal(&m_cond);
    }
};



#endif