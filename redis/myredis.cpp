#include "myredis.h"
#include <iostream>
using namespace std;
Redis::Redis(/* args */)
{
    m_connect = redisConnect("127.0.0.1", 6379);
    if(m_connect->err){
        redisFree(m_connect);
        cout << "connect to redisServer fail" << endl;
        exit(1);
    }
    string password = "123456";
    redisReply *pRedisReply = (redisReply*)redisCommand(m_connect, "AUTH %s", password.data());
    if (NULL != pRedisReply)
    {
        freeReplyObject(pRedisReply);
    }
    cout << "connect to redisServer success" << endl;
}

Redis::~Redis()
{
    if(m_connect!=nullptr){
        redisFree(m_connect);
        m_connect = nullptr;
    }
}

Redis* Redis::getInstance(){
    static Redis m_redis;
    return &m_redis;
}

bool Redis::set(string key, string val){
    redisReply *pReply = (redisReply*)redisCommand(m_connect, "SET %s %s", key.c_str(),val.data());
    string ret = pReply->str;
    freeReplyObject(pReply);
    if(ret!="OK"){
        cout <<"Redis: "<< key<<"="<<val<<" 插入缓存失败" << endl;
        return false;
    }
    cout <<"Redis: "<< key<<"="<<val<<" 插入缓存成功" << endl;
    return true;
}

string Redis::get(string key){
    string ret = "";
    redisReply *pReply = (redisReply *)redisCommand(m_connect, "GET %s", key.c_str());
    if(pReply->str==nullptr){
        cout <<"Redis: "<< key <<" 缓存未命中" << endl;
    }
    else{
        ret = pReply->str;
        cout << "Redis: "<<key <<" 缓存命中" << endl;
    }
    freeReplyObject(pReply);
    return ret;
}