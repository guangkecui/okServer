#include "myredis.h"

int main(){
    Redis *redis = Redis::getInstance();
    redis->set("cui", "1996");
    redis->get("cui");
    redis->get("chuan");
    return 0;
}