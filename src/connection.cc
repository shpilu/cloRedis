//
// cloRedis connection class implementation 
// version: 1.0 
// Copyright (C) 2018 James Wei (weijianlhp@163.com). All rights reserved
//

#include <boost/algorithm/string.hpp>
#include <sstream>
#include "hiredis/hiredis.h"
#include "internal/singleton.h"
#include "internal/log.h"
#include "connection.h"

namespace cloris {

RedisConnectionImpl RedisConnection::dummy_conn_impl(NULL);

RedisConnectionImpl::RedisConnectionImpl(RedisConnectionPool* pool) 
    : RedisReply(), 
      redis_context_(NULL),
      pool_(pool),
      action_count_(0) {
}

RedisConnectionImpl::~RedisConnectionImpl() {
    if (redis_context_) {
        redisFree(redis_context_);
        redis_context_ = NULL;
    }
} 

void RedisConnectionImpl::Done() {
    cLog(DEBUG, "Reclaim connection");
    Update(NULL, true, STATE_ERROR_INVOKE, NULL); 
    if (pool_) {
        pool_->Put(this);
    } else {
        BUG("RedisConnectionImpl manager do not exist");
    }
    return;
}

bool RedisConnectionImpl::Connect(const std::string& host, int port, const std::string& password, struct timeval &timeout, int db) {
    if (redis_context_) {
        this->Update(NULL, true, STATE_ERROR_INVOKE, ERR_REENTERING);
        cLog(ERROR, "internal implementation bug, redis_context is not NULL in 'Connect'");
        return false;
    }
    redis_context_ = redisConnectWithTimeout(host.c_str(), port, timeout);
    if (!redis_context_) {
        this->Update(NULL, true, STATE_ERROR_INVOKE, ERR_BAD_CONNECTION);
        cLog(ERROR, "No memory in creating new request");
        return false;
    }
    if (redis_context_->err) {
        this->Update(NULL, true, STATE_ERROR_HIREDIS, redis_context_->errstr);
        return false;
    }
    if (password.size() == 0) {
        this->Update(NULL, true, STATE_OK, "");
        return true;
    }
    this->Do("AUTH %s", password.c_str());
    this->Do("SELECT %d", db);
    return this->ok();
}

bool RedisConnectionImpl::Init(void *p, const std::string& host, int port, const std::string& password, int timeout_ms, int db) {
    RedisConnectionImpl* obj = static_cast<RedisConnectionImpl*>(p);
    struct timeval timeout = { timeout_ms / 1000, (timeout_ms % 1000) * 1000 };
    return obj->Connect(host, port, password, timeout, db);
}

RedisConnectionImpl& RedisConnectionImpl::Do(const char *format, ...) {
    va_list ap;
    va_start(ap, format);
    RedisConnectionImpl& resp = this->__Do(format, ap);
    va_end(ap);
    return resp;
}

RedisConnectionImpl& RedisConnectionImpl::__Do(const char *format, va_list ap) {
    ++this->action_count_;
    std::string value("");

    if (!redis_context_) {
        this->Update(NULL, true, STATE_ERROR_INVOKE, ERR_BAD_CONNECTION);
        return *this; 
    }
    redisReply* reply = (redisReply*)redisvCommand(redis_context_, format, ap);
    if (redis_context_->err) {
        this->Update(NULL, true, STATE_ERROR_HIREDIS, redis_context_->errstr);
        return *this; 
    }

    if (reply != NULL) {
        this->Update(reply, true, STATE_OK, "");
    } else {
        this->Update(NULL, true, STATE_ERROR_INVOKE, ERR_REPLY_NULL);
    }
    return *this;
}

RedisConnection::~RedisConnection() {
    cLog(DEBUG, "RedisConnection destructor...");
    if (impl_) {
        impl_->Done();
    }
}

RedisConnectionImpl* RedisConnection::operator->() {
    if (impl_) {
        return this->impl_;
    } else {
        return &RedisConnection::dummy_conn_impl;
    }
}

RedisConnectionImpl& RedisConnection::operator*() {
    if (impl_) {
        return *(this->impl_);
    } else {
        return RedisConnection::dummy_conn_impl;
    }
}

RedisConnection& RedisConnection::operator=(RedisConnectionImpl* connection) {
    if (this->impl_) {
        this->impl_->Done();
    }
    this->impl_ = connection;
    return *this;
}

} // namespace cloris

