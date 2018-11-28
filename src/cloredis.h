//
// cloRedis manager class definition
// version: 1.0 
// Copyright (C) 2018 James Wei (weijianlhp@163.com). All rights reserved
//

#ifndef CLORIS_CLOREDIS_H_
#define CLORIS_CLOREDIS_H_

#include "connection.h"

#define MAX_DB_NUM 16
#define MAX_SLAVE_CNT 16
#define DEFAULT_DB 0

#define CLOREDIS_SONAME libcloredis
#define CLOREDIS_MAJOR 0
#define CLOREDIS_MINOR 1

namespace cloris {

enum RedisRole {
    MASTER = 0,
    SLAVE  = 1,
};

struct ServiceAddress {
    std::string host;
    std::string full_host;
    int port;
};

class RedisManager {
public: 
    static RedisManager* instance();
    RedisManager();
    ~RedisManager();
    bool Init(const std::string& host, 
                 const std::string& password = "", 
                 int timeout_ms = DEFAULT_TIMEOUT_MS, 
                 ConnectionPoolOption* option = NULL,
                 std::string* err_msg = NULL); 
    bool InitEx(const std::string& master_host, 
                   const std::string& slave_hosts, 
                   const std::string& password = "", 
                   int timeout_ms = DEFAULT_TIMEOUT_MS,
                   ConnectionPoolOption* option = NULL,
                   std::string* err_msg = NULL); 
    RedisConnectionImpl* Get(int db = DEFAULT_DB, std::string* err_msg = NULL, RedisRole role = MASTER, int index = -1);
    void Flush();

    // TODO
    int ActiveConnectionCount(RedisRole role = MASTER);
    int ConnectionInUse(RedisRole role = MASTER);
    int ConnectionInPool(RedisRole role = MASTER);
    int slave_cnt() const { return slave_cnt_; }
private:
    void InitConnectionPool(RedisRole role, int db, int slave_slot);

    ServiceAddress master_addr_;
    std::vector<ServiceAddress> slave_addr_;
    ConnectionPoolOption option_;
    std::string password_;
    int timeout_ms_;
    bool inited_;
    int  slave_cnt_;
    RedisConnectionPool *master_[MAX_DB_NUM];
    RedisConnectionPool **slave_[MAX_DB_NUM];
};

} // namespace cloris

#endif // CLORIS_CLOREDIS_H_ 
