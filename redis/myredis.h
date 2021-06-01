#ifndef _MYREDIS_H_
#define _MYREDIS_H_
#include <string>
#include <hiredis/hiredis.h>
using namespace std;

class Redis
{
private:
    redisContext* m_connect;
    Redis();
    ~Redis();
    Redis(const Redis &) = delete;
    Redis& operator=(const Redis &) = delete;
public:
    static Redis* getInstance();
    bool set(string key, string val);
    string get(string key);
};

#endif
