//
// cloRedis connection class definition 
// class RedisConnectionImpl is an abstract of redis connection and response
// class RedisConnection is like an 'unique_ptr' to RedisConnectionImpl
// version: 1.0 
// Copyright (C) 2018 James Wei (weijianlhp@163.com). All rights reserved
//

#ifndef CLORIS_CLOREDIS_CONNECTION_H_
#define  CLORIS_CLOREDIS_CONNECTION_H_

#include "internal/connection_pool.h"
#include "reply.h"

#define DEFAULT_TIMEOUT_MS 200
#define SLOT_NUM 16  //support redis db 0-15 by default

#define ERR_NOT_INITED  "connection not inited"
#define ERR_REENTERING  "connect reentering"
#define ERR_BAD_HOST    "bad host format"
#define ERR_BAD_CONNECTION  "bad redis connection"
#define ERR_REPLY_NULL  "redisReply object is NULL"
#define ERR_MALLOC_ERROR "memory malloc error"

class redisContext;

namespace cloris {

class RedisConnectionImpl;
class RedisConnection;

typedef ConnectionPool<RedisConnectionImpl> RedisConnectionPool;

class RedisConnectionImpl : public RedisReply {
    friend RedisConnectionPool; 
    friend IdleList<RedisConnectionImpl>;
    friend RedisConnection;
public:
    static bool Init(void *p, const std::string& host, int port, const std::string& password, int timeout_ms, int db);
	RedisConnectionImpl(RedisConnectionPool*);
    RedisConnectionImpl& Do(const char *format, ...);
private:
	virtual ~RedisConnectionImpl(); // forbid allocation on stack
    bool IsRawConnection();
    RedisConnectionImpl& __Do(const char *format, va_list ap);
    bool Connect(const std::string& host, int port, const std::string& password, struct timeval &timeout, int db); 
    void Done();
    RedisConnectionImpl() = delete;
    RedisConnectionImpl(const RedisConnectionImpl&) = delete;
    RedisConnectionImpl& operator=(const RedisConnectionImpl&) = delete;

	redisContext* redis_context_;
    RedisConnectionPool* pool_;
    int action_count_;
};

class RedisConnection {
    static RedisConnectionImpl dummy_conn_impl;
public:
    RedisConnection(RedisConnectionImpl* conn) : impl_(conn) { }
    virtual ~RedisConnection();

    // cast RedisConnection to bool type
    operator bool() const { return impl_ ? true : false; }
    RedisConnection& operator=(RedisConnectionImpl* connection);
    RedisConnectionImpl* operator->();
    RedisConnectionImpl& operator*();

    RedisConnection(RedisConnection&&); 
    RedisConnection& operator=(RedisConnection&&);

    RedisConnectionImpl* mutable_impl() { return impl_; }
    void set_impl(RedisConnectionImpl* impl) { impl_ = impl; }
private:
    RedisConnection() = delete; 
    RedisConnection(const RedisConnection&) = delete; 
    RedisConnection& operator=(const RedisConnection&) = delete;
    RedisConnectionImpl* impl_;
};

} // namespace cloris

#endif // CLORIS_CLOREDIS_CONNECTION_H_
