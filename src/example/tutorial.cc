// 
// The following tutorial shows basic usage of cloRedis 
//

#include <iostream>
#include <memory>
#include "cloredis.h"

using namespace cloris;

static const char* g_host_master = "172.17.224.212:6379";
static const char* g_host_slaves = "172.17.224.212:6379";
static const char* g_password    = "cloris520";
static int g_timeout_ms    = 100;


void AccessRedisSimple() {
    RedisManager *manager = RedisManager::instance();
    if (!manager->Init(g_host_master, g_password, g_timeout_ms)) {
        std::cout << "init redis manager failed" << std::endl;
        return;
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
    std::unique_ptr<RedisManager> manager = std::unique_ptr<RedisManager>(new RedisManager());
    if (!manager->InitEx(g_host_master, g_host_slaves, g_password, g_timeout_ms)) {
        std::cout << "init redis manager failed" << std::endl;
        return;
    }
    RedisConnection conn1 = manager->Get(2, NULL, MASTER);
    conn1->Do("SET phonenum %s", "1891189xxxx");
    sleep(5);
    // You can also use the following code to get a connection to slave redis 
    // RedisConnection conn2 = manager->Get(2, NULL, SLAVE, 0);
    // RedisConnection conn2 = manager->Get(2, NULL, SLAVE, 1);
    RedisConnection conn2 = manager->Get(2, NULL, SLAVE);
    std::cout << conn2->Do("GET phonenum").toString() << std::endl; // 1891189xxxx
}

void AccessRedisWithErrorCheck() {
    ConnectionPoolOption option;
    option.max_idle = 40;
    option.max_active = 100;
    option.idle_timeout_ms = 180 * 1000;   
    std::unique_ptr<RedisManager> manager(new RedisManager());
    if (!manager->InitEx(g_host_master, g_host_slaves, g_password, g_timeout_ms, &option)) {
        std::cout << "init redis manager failed" << std::endl;
        return;
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
}

void AccessRedisByHgetAll() {
    std::unique_ptr<RedisManager> manager(new RedisManager());
    if (!manager->Init(g_host_master, g_password, g_timeout_ms)) {
        std::cout << "init redis manager failed" << std::endl;
        return;
    }
    cloris::RedisConnection conn = manager->Get(6);
    conn->Do("HSET language %s %s", "C", "leve13");
    conn->Do("HSET language %s %s", "C++", "level2");
    conn->Do("HSET language %s %s", "Java", "level4");
    conn->Do("HSET language %s %s", "PHP", "leve4");
    conn->Do("HSET language %s %s", "Python", "level5");
    conn->Do("HSET language %s %s", "Golang", "leve7");

    // run HGETALL command
    conn->Do("HGETALL language");

    // Both demos below are OK for array-type redisReply traversing.
    // It's noteworthy that "RedisReply" MUST BE use in the life time of "RedisConnection",
    // (that is, RedisReply must be use before RedisConnection is destructed)
    // code fragment likes below is strictly forbidden!
    //
    // RedisReply reply;
    // {
    //     RedisConnection conn = manager->Get(6);
    //     reply = conn->get(0); 
    //     ...
    //     // object 'conn' will destruct here    
    // }
    //  ...
    // std::string value = reply.toString(); // unpredictable result
    // 
    // demo1
    std::cout << "demo1 ==>" << std::endl;
    for (size_t i = 0; i < conn->size(); ++i) {
        cloris::RedisReply reply = conn->get(i);
        std::cout << reply.toString() << std::endl;        
    }

    // demo2
    std::cout << "demo2 ==>" << std::endl;
    for (size_t i = 0; i < conn->size(); ++i) {
        std::cout << conn->get(i).toString() << std::endl;        
    }
}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    AccessRedisSimple();
    AccessMasterSlave();
    AccessRedisWithErrorCheck();
    AccessRedisByHgetAll();
    return 0;
}

