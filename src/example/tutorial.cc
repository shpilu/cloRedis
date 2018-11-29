// 
// The following tutorial shows basic usage of cloRedis 
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
    std::cout << conn1->Do("GET tkey1").toString() << std::endl; // 100
    std::cout << conn1->Do("GET tkey1").toInt32() << std::endl; // 100

    // case 2:
    conn1 = manager->Get(2);
    conn1->Do("SET mykey %s", "This is a test");
    std::cout << conn1->Do("GET mykey").toString() << std::endl;  // This is a test

    // case 3:
    RedisConnection conn2 = manager->Get(3);
    conn2->Do("HSET hkey f1 %d", 123);
}

void AccessMasterSlave() {
    RedisManager *manager = new RedisManager();
    if (!manager->InitEx("172.17.224.212:6379", "172.17.224.212:6380,172.17.224.212:6381", "cloris520", 100)) {
        std::cout << "init redis manager failed" << std::endl;
    }
    RedisConnection conn1 = manager->Get(2, NULL, MASTER);
    conn1->Do("SET phonenum %s", "1891189xxxx");
    sleep(5);
    // You can also use the following code to get a connection to slave redis 
    // RedisConnection conn2 = manager->Get(2, NULL, SLAVE, 0);
    // RedisConnection conn2 = manager->Get(2, NULL, SLAVE, 1);
    RedisConnection conn2 = manager->Get(2, NULL, SLAVE);
    std::cout << conn2->Do("GET phonenum").toString() << std::endl; // 1891189xxxx
    delete manager;
}

void AccessRedisWithErrorCheck() {
    ConnectionPoolOption option;
    option.max_idle = 40;
    option.max_active = 100;
    option.idle_timeout_ms = 180 * 1000;   
    RedisManager *manager = new RedisManager();
    if (!manager->InitEx("172.17.224.212:6379", "172.17.224.212:6380,172.17.224.212:6381", "cloris520", 100, &option)) {
        std::cout << "init redis manager failed" << std::endl;
    }
    RedisConnection conn1 = manager->Get(5);
    conn1->Do("NOCOMMAND city %s", "Beijing");
    // use 'ok' or 'error' method to determine whether the command has run success
    if (conn1->error()) {
        // output: ERR unknown command `NOCOMMAND`, with args beginning with: `city`, `Beijing`
        std::cout << "run command error:" << conn1->err_str() << std::endl;
    } else {
        std::cout << "run command ok" << std::endl;
    }
    delete manager;
}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    AccessRedisSimple();
    AccessMasterSlave();
    AccessRedisWithErrorCheck();
    return 0;
}

