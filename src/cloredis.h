// Cloredis is a simple, flexsible, easy-to-use C++ client library for the Redis database
// Copyright 2018 James Wei (weijianlhp@163.com)
//
// usage:
// RedisManager manager = RedisManager::instance();
// manager->Connect("127.0.0.1", 6379, "password"); 
// RedisConnection conn = manager->Get(1);
// std::string value = conn->Do("GET %s", key).toString();
// bool exist = conn->Do("EXISTS %s", key).toBool();
// conn->Select(3);
// conn->Do("SET %s %d", "my_key", 8);
// ...
//

#ifndef CLORIS_REDIS_H_
#define CLORIS_REDIS_H_

#include <string>
#include <memory>
#include <mutex>
#include <deque>

#define CLOREDIS_SONAME libcloredis
#define CLOREDIS_MAJOR 0
#define CLOREDIS_MINOR 1

#define REDIS_ERRSTR_LEN 256
#define SLOT_NUM 16  //support redis db 0-15 by default

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

class RedisReply;
class RedisManager;
class RedisConnection;

typedef std::shared_ptr<RedisReply> RedisReplyPtr;

class RedisReply {
public:
    RedisReply(redisReply* reply, bool reclaim, ERR_STATE state, const char* err_msg, bool copy_errstr);
    ~RedisReply();

    void Update(redisReply* rep, bool reclaim, ERR_STATE state, const char* err_msg, bool copy_errstr); 

    ERR_STATE err_state() const { return err_state_; }
    bool error() const; 
    bool ok() const;
    std::string toString() const;
    int32_t toInt32() const;
    int64_t toInt64() const;
    std::string err_str() const;
    int type() const;
    bool is_nil() const;
    bool is_string() const;
    bool is_int() const;
    bool is_array() const;
    size_t size() const;

    RedisReplyPtr operator[](size_t index);
private:
    void UpdateErrMsg(const char* err_msg, bool copy_errstr); 
    RedisReply() = delete;
    char err_str_[REDIS_ERRSTR_LEN]; //
    redisReply* reply_;
    const char *err_msg_;
    ERR_STATE err_state_;
    bool reclaim_;
};

class RedisConnectionImpl {
public:
    friend class RedisManager; 
    friend class RedisConnection;
public:
	RedisConnectionImpl(RedisManager*);
	~RedisConnectionImpl();

    RedisConnectionImpl& Do(const char *format, ...);
    bool DoBool(const char *format, ...);
    bool Select(int db);
	void Close();
    int db() const { return db_; }

    ERR_STATE err_state() const; 
    bool error() const; 
    bool ok() const;
    std::string toString() const;
    int32_t toInt32() const;
    int64_t toInt64() const;
    std::string err_str() const;
    int type() const;
    bool is_nil() const;
    bool is_string() const;
    bool is_int() const;
    bool is_array() const;
    size_t size() const;

    RedisReplyPtr Reply();
private:
    RedisConnectionImpl& __Do(const char *format, va_list ap);

    bool Connect(const std::string& host, int port , struct timeval &timeout, const std::string& password);
    void UpdateProcessState(redisReply* rep, bool reclaim, ERR_STATE state, const char* err_msg, bool copy_errstr = false); 
    bool ReclaimOk(int action_limit);
    void Done();
private:
    RedisConnectionImpl() = delete;
    RedisConnectionImpl(const RedisConnectionImpl&) = delete;
    RedisConnectionImpl& operator=(const RedisConnectionImpl&) = delete;
	redisContext* redis_context_;
    RedisManager* manager_;
    RedisReplyPtr reply_ptr_;
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
public: 
    static RedisManager* instance();
    RedisManager(const std::string& host, int port, int timeout_ms, const std::string& password = "");
    RedisManager();
    ~RedisManager();

    bool Connect(const std::string& host, int port, int timeout_ms, const std::string& password = ""); 
    RedisConnectionImpl& Do(int db, const char *format, ...);
    RedisConnectionImpl* Get(int db);
    const std::string& err_str() const { return err_str_; }
    int conn_in_use() const { return conn_in_use_; }
    int conn_in_pool() const { return conn_in_pool_; }
private:
    friend class RedisConnectionImpl;
    void Gc(RedisConnectionImpl* connection);
    void AddErrorRecord(RedisConnectionImpl* conn);

    ConnectionQueue queue_[SLOT_NUM];
    std::string err_str_;
    std::string host_;
    int port_;
    struct timeval timeout_;
    std::string password_;
    int action_limit_;
    uint64_t time_limit_;
    int conn_in_use_;
    int conn_in_pool_;
};

} // namespace cloris

#endif // CLORIS_REDIS_H_
