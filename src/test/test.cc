//
// Copyright (c) 2018 James Wei (weijianlhp@163.com). All rights reserved.
// Cloredis unit test based on Google Test 
//

#include <gtest/gtest.h>
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

TEST(cloredis, test1) {
    std::string host     = Config::instance()->getString("redis.host");
    int32_t port         = Config::instance()->getInt32("redis.port");
    int32_t timeout      = Config::instance()->getInt32("redis.timeout");
    std::string password = Config::instance()->getString("redis.password");

    RedisManager* manager = RedisManager::instance();
    ASSERT_TRUE(manager->Connect(host, port, timeout, password)) << manager->err_str();
    ASSERT_EQ(1, manager->conn_in_pool());

    std::string value;
    {
        const char *test_str = "This is just a test";
        RedisConnection conn = manager->Get(2);
        ASSERT_EQ(1, manager->conn_in_use());
        ASSERT_EQ(1, manager->conn_in_pool());
        ASSERT_STREQ("OK", conn->Do("SET m_key %s", test_str).toString().c_str());
        value = conn->Do("GET %s", "m_key").toString();
        ASSERT_STREQ(test_str, value.c_str());
    }

    ASSERT_EQ(0, manager->conn_in_use());
    ASSERT_EQ(2, manager->conn_in_pool()); // 2 connections in db0 and db2

    {
        RedisConnection conn1 = manager->Get(1);
        ASSERT_STREQ("OK", conn1->Do("SET m_kval %s", "test_val").toString().c_str());
        ASSERT_EQ(1, conn1->Do("DEL m_kval").toInt32());
        ASSERT_TRUE(conn1->Do("GET m_val").is_nil());
    }

    ASSERT_EQ(0, manager->conn_in_use());
    ASSERT_EQ(3, manager->conn_in_pool()); // 3 connections in db0, db1 and db2

    {
        RedisConnection cons = manager->Get(2);
        ASSERT_EQ(1, manager->conn_in_use());
        ASSERT_EQ(2, manager->conn_in_pool()); 
        ASSERT_STREQ("", cons->Do("GET %s", "m_version").toString().c_str());
    }
    ASSERT_EQ(0, manager->conn_in_use());
    ASSERT_EQ(3, manager->conn_in_pool()); // 3 connections in db0, db1 and db2
}

int main(int argc, char** argv) {
    // use cloriConf to load config
    Config::instance()->Load(g_redis_conf, SP_DIRECT);
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
