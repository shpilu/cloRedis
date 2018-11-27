//
// cloRedis connection header file
// Copyright 2018 James Wei (weijianlhp@163.com)
//

#ifndef CLORIS_REDIS_H_
#define CLORIS_REDIS_H_

#include <string>
#include <memory>
#include <mutex>
#include "macro.h"

#define DEFAULT_TIMEOUT_MS 200
#define REDIS_ERRSTR_LEN 256
#define SLOT_NUM 16  //support redis db 0-15 by default

#define ERR_NOT_INITED  "connection not inited"
#define ERR_REENTERING  "connect reentering"
#define ERR_NO_CONTEXT  "no redisContext object"
#define ERR_REPLY_NULL  "redisReply object is NULL"

class redisContext;
class redisReply;

namespace cloris {

class RedisReply;
class RedisConnectionImpl;

typedef ConnectionPool<RedisConnectionImpl> RedisConnectionPool;

class RedisConnectionImpl : public GenericConnection, public RedisReply {
    friend class RedisConnectionPool; 
public:
	RedisConnectionImpl(RedisConnectionPool*);
    RedisConnectionImpl& Do(const char *format, ...);
    bool Select(int db);
    int db() const { return db_; }

private:
	~RedisConnectionImpl(); // forbid allocation in the stack
    RedisConnectionImpl& __Do(const char *format, va_list ap);
    bool Connect(const std::string& host, int port , struct timeval &timeout, const std::string& password);
    void UpdateProcessState(redisReply* rep, bool reclaim, ERR_STATE state, const char* err_msg, bool copy_errstr = false); 
    void Done();

    RedisConnectionImpl() = delete;
    RedisConnectionImpl(const RedisConnectionImpl&) = delete;
    RedisConnectionImpl& operator=(const RedisConnectionImpl&) = delete;

	redisContext* redis_context_;
    RedisConnectionPool* pool_;
    int action_count_;
    int db_;
};

// RedisConnection: guard of RedisConnectionImpl
class RedisConnection {
public:
    RedisConnection(RedisConnectionImpl* conn) : impl_(conn) { }
    ~RedisConnection();

    RedisConnection& operator=(RedisConnectionImpl* connection);
    bool operator!();
    RedisConnectionImpl* operator->();
    RedisConnectionImpl& operator*();
private:
    RedisConnection() = delete; 
    RedisConnectionImpl* impl_;
};
