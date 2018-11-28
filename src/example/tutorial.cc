// 
// The following shows basic usage of cloRedis 
//

#include <iostream>
#include "cloredis.h"

using namespace cloris;

void AccessRedisSimple() {
    RedisManager *manager = RedisManager::instance();
    if (!manager->Init("172.17.224.212:6379", "cloris520", 100)) {
        std::cout << "init redis manager failed" << std::endl;
    }

    // case 1:
    RedisConnection conn1 = manager->Get(1);
    conn1->Do("SET tkey1 %d", 100);
    std::cout << conn1->Do("GET tkey1").toString() << std::endl;
    std::cout << conn1->Do("GET tkey1").toInt32() << std::endl;

    // case 2:
    conn1 = manager->Get(2);
    conn1->Do("SET mykey %s", "This is a test");
    std::cout << conn1->Do("GET mykey").toString() << std::endl; 

    // case 3:
    RedisConnection conn2 = manager->Get(3);
    conn2->Do("HSET hkey f1 %d", 123);
}

int main(int argc, char** argv) {
    AccessRedisSimple();
    return 0;
}

