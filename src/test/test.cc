#include <iostream>
#include "config.h"
#include "cloredis.h"

using namespace cloris;

const char *g_redis_conf = " \
[redis] \n\
host=192.168.19.229 \n\
port=6379 \n\
timeout=1000\n\
password=weijian\n\
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
    std::cout << "test begin=" << std::endl;

    std::string value;
    {
        RedisConnection conn = manager->Get(2);
        std::cout << "test begin=#" << std::endl;
        conn->Do("SET m_key %s", "This is just a test");
        std::cout << "test begin=##" << std::endl;
        value = conn->Do("GET %s", "m_key").toString();
        std::cout << "test begin=###" << std::endl;
        std::cout << "m_key=" << value << std::endl;
    }

    std::cout << "manager Do test, m_Key=" << manager->Do(2, "GET %s", "m_key").toString() << std::endl;

    {
        RedisReplyPtr reply;
        {
            RedisConnection conn1 = manager->Get(1);
            std::string ret = conn1->Do("SET m_kval %s", "test_val").toString();
            std::cout << "SET m_kval=" << ret << std::endl;
            
            conn1->Do("DEL m_kval");
            reply = conn1->Do("GET m_val").Reply();
            std::cout << "after del, m_kval=" << reply->toString() << std::endl;
            if (reply->is_nil()) {
                std::cout << "after deleting, m_kval is nil" << std::endl;
            }
        }
        std::cout << "test_redis_reply##=" << reply->toString() << std::endl;
    }

    {
        RedisConnection cons = manager->Get(2);
        std::string val = cons->Do("GET %s", "m_version").toString();
        std::cout << "m_version=" << val << std::endl;
    }
}

int main() {
    Config::instance()->Load(g_redis_conf, SP_DIRECT);
    test_get();
}
