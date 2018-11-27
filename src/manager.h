//
// cloRedis  
// Copyright 2018 James Wei (weijianlhp@163.com)
//

#ifndef CLORIS_CLOREDIS_MANAGER_H_
#define CLORIS_CLOREDIS_MANAGER_H_

#include "connection.h"

#define MAX_DB_NUM 16
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
    /*
    bool InitEx(const std::string& master_host, 
                   const std::string& slave_hosts, 
                   const std::string& password = "", 
                   int timeout_ms = DEFAULT_TIMEOUT_MS,
                   ConnectionOption* option = NULL,
                   std::string* err_msg = NULL); 
    */
    RedisConnectionImpl* Get(int db, std::string* err_msg = NULL, RedisRole role = MASTER, int index = -1);
    void Flush();
private:
    bool inited_;
    int  slave_cnt_;
    RedisConnectionPool *master_[MAX_DB_NUM];
    // RedisConnectionPool **slave_[MAX_DB_NUM];
};

} // namespace cloris

#endif // CLORIS_CLOREDIS_MANAGER_H_ 
