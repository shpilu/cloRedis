// Cloredis, a simple, flexsible, easy-to-use C++ client library for the Redis database based on hiredis
// Copyright 2018 James Wei (weijianlhp@163.com)
//
// usage:
// RedisManager manager("127.0.0.1", 6379, "password"); 
// RedisConnectionGuard conn = manager->Get(1);
// std::string value = conn->Do("GET %s", key).toString();
// bool exist = conn->Do("EXISTS %s", key).toBool();
// connection->Select(3);
// connection->Do("SET %s %d", "my_key", 8);
// ...
// (connection releasing is not necessary, RedisConnectionGuard class will do it automatically in its destructor function)
//

#ifndef CLORIS_REDIS_H_
#define CLORIS_REDIS_H_

#include <string>
#include <mutex>
#include <deque>

#define CLOREDIS_SONAME libcloredis
#define CLOREDIS_MAJOR 0
#define CLOREDIS_MINOR 1

#define ERR_NOT_INITED  "connection not inited"
#define ERR_REENTERING  "connect reentering"
#define ERR_NO_MEMORY   "no memory"
#define ERR_NO_REPLY    "no reply"

class redisContext;

namespace cloris {

enum ERR_STATE {
    STATE_OK = 0,
    STATE_ERROR_TYPE = 1,
    STATE_ERROR_COMMAND = 2,
    STATE_ERROR_HIREDIS = 3,
    STATE_ERROR_INVOKE = 4,
};

struct RedisState {
    RedisState() : es(STATE_OK), msg("") { }

    ERR_STATE es;
    std::string msg;
};

class RedisManager;
class RedisConnectionGuard;

class RedisConnection {
public:
    friend class RedisManager; 
    friend class RedisConnectionGuard;
public:
	RedisConnection(RedisManager*);
	~RedisConnection();

    std::string Do(const char *format, ...);
    bool DoBool(const char *format, ...);
    bool Select(int db);
    bool Ok() const; 
    ERR_STATE ErrorCode() const; 
    const std::string& ErrorString() const;
	void Close();
    int db() const { return db_; }
private:
    bool Connect(const std::string& host, int port , struct timeval &timeout, const std::string& password);
    void UpdateProcessState(ERR_STATE state, const std::string& msg);
    bool ReclaimOk(int action_limit);
    void Done();
private:
    RedisConnection(const RedisConnection&) = delete;
    RedisConnection& operator=(const RedisConnection&) = delete;
	redisContext* redis_context_;
    RedisManager* manager_;
    RedisState state_;
    uint64_t access_time_;
    int action_count_;
    int db_;
};

class RedisConnectionGuard {
public:
    RedisConnectionGuard() : connection_(NULL) { }
    RedisConnectionGuard(RedisConnection* conn) : connection_(conn) { }
    ~RedisConnectionGuard();

    RedisConnection* operator->();
    RedisConnection& operator*();
    RedisConnectionGuard& operator=(RedisConnection* connection);

private:
    RedisConnection* connection_;
};

struct ConnectionQueue {
    std::mutex queue_mtx;
    std::deque<RedisConnection*> conns;
};

class RedisReplyGuard {
    
};

class RedisManager {
    #define SLOT_NUM 16
public: 
    RedisManager();
    ~RedisManager();

    RedisManager(const std::string& host, int port, int timeout_ms, const std::string& password = "");
    bool Connect(const std::string& host, int port, int timeout_ms, const std::string& password = ""); 

    RedisConnection* Get(int db);
    static RedisManager* instance();
private:
    friend class RedisConnection;
    void Gc(RedisConnection* connection);

    ConnectionQueue queue_[SLOT_NUM];
    std::string host_;
    int port_;
    struct timeval timeout_;
    std::string password_;
    int action_limit_;
    uint64_t time_limit_;
};

} // namespace cloris

#endif // CLORIS_REDIS_H_
