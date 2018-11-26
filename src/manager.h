//
// cloRedis  
// Copyright 2018 James Wei (weijianlhp@163.com)
//

#ifndef CLORIS_CLOREDIS_MANAGER_H_
#define CLORIS_CLOREDIS_MANAGER_H_

#include "connection.h"

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
    bool Connect(const std::string& host, 
                 const std::string& password = "", 
                 int timeout_ms = DEFAULT_TIMEOUT_MS, 
                 ConnectionOption* option = NULL,
                 std::string* err_msg = NULL); 
    bool ConnectEx(const std::string& master_host, 
                   const std::string& slave_hosts, 
                   const std::string& password = "", 
                   int timeout_ms = DEFAULT_TIMEOUT_MS,
                   ConnectionOption* option = NULL,
                   std::string* err_msg = NULL); 
    RedisConnectionImpl* Get(int db, RedisRole role = MASTER);
    void Flush();
pivate:
    bool inited_;
    int  slave_cnt_;
    RedisConnectionPool *master_;
    RedisConnectionPool **slave_;
};

} // namespace cloris

#endif // CLORIS_CLOREDIS_MANAGER_H_ 
