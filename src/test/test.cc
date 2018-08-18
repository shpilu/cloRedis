#include <iostream>
#include "config.h"
#include "cloredis.h"

using namespace cloris;

const char *g_redis_conf = " \
[redis] \n\
host=192.168.19.229 \n\
port=6379 \n\
timeout=3000\n\
password=weijian \n\
";

void test_get() {
    std::string host     = Config::instance()->getString("redis.host");
    int32_t port         = Config::instance()->getInt32("redis.port");
    int32_t timeout      = Config::instance()->getInt32("redis.timeout");
    std::string password = Config::instance()->getString("redis.password");

    RedisManager* manager = RedisManager::instance();

    std::cout << "host=" << host << ", port=" << port << ",timeout=" << timeout << ",password=" << password << std::endl;
    if (!manager->Connect(host, port, timeout, password)) {
        std::cout << "[ERROR] redis init error, msg=xx" << std::endl;
        return;
    }

    std::string value;
    {
        RedisConnectionGuard conn = manager->Get(2);
        value = conn->Do("GET %s", "m_yueka");
    }
    std::cout << "m_yueka=" << value  << std::endl;
    std::string kval; 
    {
        RedisConnectionGuard conn1 = manager->Get(1);
        kval = conn1->Do("GET h132");
    }
    std::cout << "After do...=" << std::endl;
    std::cout << "test_key=" << kval << std::endl;
    {
        RedisConnectionGuard cons = manager->Get(2);
        std::string val = cons->Do("GET %s", "m_yimachuangxin");
        std::cout << "m_yimachuangxin=" << val << std::endl;
    }
}

int main() {
    Config::instance()->Load(g_redis_conf, SP_DIRECT);
    test_get();
}
