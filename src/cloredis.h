// Cloredis, a simple, flexsible, easy-to-use C++ client library for the Redis database based on hiredis
// Copyright 2018 James Wei (weijianlhp@163.com)
//
// usage:
// RedisManager manager = RedisManager::instance(); //
// manager->Connect("127.0.0.1", 6379, "password"); 
// RedisConnection conn = manager->Get(1);
// std::string value = conn->Do("GET %s", key).toString();
// bool exist = conn->Do("EXISTS %s", key).toBool();
// conn->Select(3);
// conn->Do("SET %s %d", "my_key", 8);
// ...
// (connection releasing is not necessary, RedisConnection class will do it automatically in its destructor function)
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
#define ERR_NO_CONTEXT  "no redisContext object"
#define ERR_REPLY_NULL  "redisReply object is NULL"

class redisContext;
class redisReply;

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

class RedisReply;
class RedisManager;
class RedisConnection;

class RedisReply {
public:
    RedisReply(redisReply* reply, bool reclaim, const char *err_msg = NULL);
    ~RedisReply();

    std::string toString() const;
    int32_t toInt32() const;
    int64_t toInt64() const;
    std::string err_str() const;
    int type() const;
    bool is_error() const;
    bool is_nil() const;
    bool is_string() const;
    bool is_int() const;
    bool is_array() const;
    size_t size() const;

    RedisReply operator[](size_t index);

private:
    RedisReply() = delete;
    redisReply* reply_;
    bool reclaim_;
    const char *err_msg_;
};

class RedisConnectionImpl {
public:
    friend class RedisManager; 
    friend class RedisConnection;
public:
	RedisConnectionImpl(RedisManager*);
	~RedisConnectionImpl();

    RedisReply Do(const char *format, ...);
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
    RedisConnectionImpl() = delete;
    RedisConnectionImpl(const RedisConnectionImpl&) = delete;
    RedisConnectionImpl& operator=(const RedisConnectionImpl&) = delete;
	redisContext* redis_context_;
    RedisManager* manager_;
    RedisState state_;
    uint64_t access_time_;
    int action_count_;
    int db_;
};

class RedisConnection {
public:
    RedisConnection() = delete; 
    RedisConnection(RedisConnectionImpl* conn) : impl_(conn) { }
    ~RedisConnection();

    RedisConnectionImpl* operator->();
    RedisConnectionImpl& operator*();
    RedisConnection& operator=(RedisConnectionImpl* connection);

private:
    RedisConnectionImpl* impl_;
};

struct ConnectionQueue {
    std::mutex queue_mtx;
    std::deque<RedisConnectionImpl*> conns;
};

class RedisManager {
    #define SLOT_NUM 16
public: 
    RedisManager();
    ~RedisManager();

    RedisManager(const std::string& host, int port, int timeout_ms, const std::string& password = "");
    bool Connect(const std::string& host, int port, int timeout_ms, const std::string& password = ""); 

    RedisConnectionImpl* Get(int db);
    static RedisManager* instance();
private:
    friend class RedisConnectionImpl;
    void Gc(RedisConnectionImpl* connection);

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
