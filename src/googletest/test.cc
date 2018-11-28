//
// Cloredis unit test based on Google Test 
// Copyright (c) 2018 James Wei (weijianlhp@163.com). All rights reserved.
//

#include <gtest/gtest.h>
#include "internal/log.h"
#include "config.h"
#include "cloredis.h"

using namespace cloris;

const char *g_redis_conf = " \
[redis] \n\
host=172.17.224.212:6379 \n\
slave_host=172.17.224.212:6380,172.17.224.212:6381\n\
timeout=1000\n\
password=cloris520 \n\
";

TEST(cloredis, basic_test) {
    std::string host     = Config::instance()->getString("redis.host");
    int32_t timeout      = Config::instance()->getInt32("redis.timeout");
    std::string password = Config::instance()->getString("redis.password");
    cLog(INFO, "[TEST_BEGIN]host=%s, timeout=%d, password=%s", host.c_str(), timeout, password.c_str());

    RedisManager* manager = RedisManager::instance();
    ASSERT_FALSE(manager->Init("localhost:80", password, timeout));
    manager->Flush();
    ASSERT_TRUE(manager->Init(host, password, timeout));
    ASSERT_EQ(1, manager->ConnectionInPool());

    std::string value;
    {
        const char *test_str = "This is just a test";
        RedisConnection conn = manager->Get(2);
        ASSERT_EQ(1, manager->ConnectionInUse());
        ASSERT_EQ(1, manager->ConnectionInPool());
        ASSERT_EQ(2, manager->ActiveConnectionCount());
        ASSERT_STREQ("OK", conn->Do("SET m_key %s", test_str).toString().c_str());
        ASSERT_TRUE(conn->ok());
        value = conn->Do("GET %s", "m_key").toString();
        ASSERT_STREQ(test_str, value.c_str());
        ASSERT_TRUE(conn->ok());
    }

    ASSERT_EQ(0, manager->ConnectionInUse());
    ASSERT_EQ(2, manager->ConnectionInPool()); // 2 connections in db0 and db2

    {
        RedisConnection conn1 = manager->Get(1);
        ASSERT_STREQ("OK", conn1->Do("SET m_kval %s", "test_val").toString().c_str());
        ASSERT_EQ(1, conn1->Do("DEL m_kval").toInt32());
        ASSERT_TRUE(conn1->Do("GET m_val").is_nil());
    }

    ASSERT_EQ(0, manager->ConnectionInUse());
    ASSERT_EQ(3, manager->ConnectionInPool()); // 3 connections in db0, db1 and db2

    {
        RedisConnection cons = manager->Get(2);
        ASSERT_EQ(1, manager->ConnectionInUse());
        ASSERT_EQ(2, manager->ConnectionInPool()); 
        ASSERT_STREQ("", cons->Do("GET %s", "m_version").toString().c_str());
    }
    ASSERT_EQ(0, manager->ConnectionInUse());
    ASSERT_EQ(3, manager->ConnectionInPool()); // 3 connections in db0, db1 and db2
}

TEST(cloredis, max_active_test) {
    std::string host     = Config::instance()->getString("redis.host");
    int32_t timeout      = Config::instance()->getInt32("redis.timeout");
    std::string password = Config::instance()->getString("redis.password");

    ConnectionPoolOption option;
    option.max_active = 4;
    
    RedisManager* manager = new RedisManager();
    ASSERT_TRUE(manager->Init(host, password, timeout, &option));
    ASSERT_EQ(1, manager->ConnectionInPool());
    {
        RedisConnection conn1 = manager->Get(2);
        ASSERT_TRUE(conn1);
        RedisConnection conn2 = manager->Get(2);
        ASSERT_TRUE(conn2);
        RedisConnection conn3 = manager->Get(2);
        ASSERT_TRUE(conn3);
        RedisConnection conn4 = manager->Get(2);
        ASSERT_TRUE(conn4);
        RedisConnection conn5 = manager->Get(2);
        ASSERT_FALSE(conn5);
        RedisConnection conn6 = manager->Get(2);
        ASSERT_FALSE(conn6);
        RedisConnection conn7 = manager->Get(1);
        ASSERT_TRUE(conn7);
    }
    ASSERT_EQ(6, manager->ConnectionInPool());
    delete manager;
}

TEST(cloredis, max_idle_test) {
    std::string host     = Config::instance()->getString("redis.host");
    int32_t timeout      = Config::instance()->getInt32("redis.timeout");
    std::string password = Config::instance()->getString("redis.password");

    ConnectionPoolOption option;
    option.max_idle = 3;
    
    RedisManager* manager = new RedisManager();
    ASSERT_TRUE(manager->Init(host, password, timeout, &option));
    ASSERT_EQ(1, manager->ConnectionInPool());
    {
        RedisConnection conn1 = manager->Get(2);
        ASSERT_TRUE(conn1);
        RedisConnection conn2 = manager->Get(2);
        ASSERT_TRUE(conn2);
        RedisConnection conn3 = manager->Get(2);
        ASSERT_TRUE(conn3);
        RedisConnection conn4 = manager->Get(2);
        ASSERT_TRUE(conn4);
        RedisConnection conn5 = manager->Get(2);
        ASSERT_TRUE(conn5);
        RedisConnection conn6 = manager->Get(2);
        ASSERT_TRUE(conn6);
    }
    ASSERT_EQ(4, manager->ConnectionInPool());
    delete manager;
}

TEST(cloredis, idle_timeout_ms) {
    std::string host     = Config::instance()->getString("redis.host");
    int32_t timeout      = Config::instance()->getInt32("redis.timeout");
    std::string password = Config::instance()->getString("redis.password");

    ConnectionPoolOption option;
    option.idle_timeout_ms = 2000;
    
    RedisManager* manager = new RedisManager();
    ASSERT_TRUE(manager->Init(host, password, timeout, &option));
    {
        RedisConnection conn1 = manager->Get(0);
        ASSERT_TRUE(conn1);
        RedisConnection conn2 = manager->Get(0);
        ASSERT_TRUE(conn2);
        RedisConnection conn3 = manager->Get(0);
        ASSERT_TRUE(conn3);
    }
    ASSERT_EQ(3, manager->ConnectionInPool());
    sleep(3);
    ASSERT_EQ(3, manager->ConnectionInPool());
    {
        RedisConnection conn4 = manager->Get(0);
        ASSERT_TRUE(conn4);
    }
    ASSERT_EQ(1, manager->ConnectionInPool());
    delete manager;
}

TEST(cloredis, max_conn_life_time) {
    std::string host     = Config::instance()->getString("redis.host");
    int32_t timeout      = Config::instance()->getInt32("redis.timeout");
    std::string password = Config::instance()->getString("redis.password");

    ConnectionPoolOption option;
    option.max_conn_life_time = 3000;
    
    RedisManager* manager = new RedisManager();
    ASSERT_TRUE(manager->Init(host, password, timeout, &option));
    {
        RedisConnection conn1 = manager->Get(1);
        ASSERT_TRUE(conn1);
        RedisConnection conn2 = manager->Get(1);
        ASSERT_TRUE(conn2);
    }
    ASSERT_EQ(3, manager->ConnectionInPool());
    sleep(2);
    {
        RedisConnection conn1 = manager->Get(1);
        ASSERT_TRUE(conn1);
        RedisConnection conn2 = manager->Get(1);
        ASSERT_TRUE(conn2);
    }
    ASSERT_EQ(3, manager->ConnectionInPool());
    sleep(2);
    {
        RedisConnection conn1 = manager->Get(1);
        ASSERT_TRUE(conn1);
        RedisConnection conn2 = manager->Get(1);
        ASSERT_TRUE(conn2);
    }
    // the expired connection in db 0 has not been kick off
    ASSERT_EQ(3, manager->ConnectionInPool());
    delete manager;
}

TEST(cloredis, slave_test) {
    std::string host     = Config::instance()->getString("redis.host");
    std::string slave_host = Config::instance()->getString("redis.slave_host"); 
    int32_t timeout      = Config::instance()->getInt32("redis.timeout");
    std::string password = Config::instance()->getString("redis.password");

    RedisManager* manager = new RedisManager();
    ASSERT_TRUE(manager->InitEx(host, slave_host, password, timeout));
    ASSERT_EQ(2, manager->slave_cnt());
    {
        RedisConnection conn1 = manager->Get(3);
        ASSERT_TRUE(conn1);
        ASSERT_TRUE(conn1->Do("SET k1 10").ok());
        RedisConnection conn2 = manager->Get(3, NULL, SLAVE);
        ASSERT_TRUE(conn2);
        ASSERT_FALSE(conn2->Do("SET k1 10").ok());
    }
    ASSERT_EQ(2, manager->ConnectionInPool(SLAVE));
    ASSERT_EQ(2, manager->ConnectionInPool(MASTER));
    sleep(4);
    {
        RedisConnection conn3 = manager->Get(3, NULL, SLAVE, 0);
        ASSERT_TRUE(conn3);
        ASSERT_EQ("10", conn3->Do("GET k1").toString());
        ASSERT_EQ(10,   conn3->Do("GET k1").toInt32());

        RedisConnection conn4 = manager->Get(3, NULL, SLAVE, 1);
        ASSERT_TRUE(conn4);
        ASSERT_EQ("10", conn4->Do("GET k1").toString());
        ASSERT_EQ(10,   conn4->Do("GET k1").toInt32());
    }
    delete manager;
}

int main(int argc, char** argv) {
    // use cloriConf to load config
    Config::instance()->Load(g_redis_conf, SP_DIRECT);
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
