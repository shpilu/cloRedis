//
// Copyright (c) 2018 James Wei (weijianlhp@163.com). All rights reserved.
// Cloredis core implementation
//
// ChangeLog:
// 2018-04-20  Created

#include <boost/algorithm/string.hpp>
#include <sstream>
#include "hiredis/hiredis.h"
#include "internal/singleton.h"
#include "internal/log.h"
#include "connection.h"

namespace cloris {

RedisConnectionImpl::RedisConnectionImpl(RedisConnectionPool* pool) 
    : RedisReply(), 
      redis_context_(NULL),
      pool_(pool),
      action_count_(0),
      db_(0) {
}

RedisConnectionImpl::~RedisConnectionImpl() {
    if (redis_context_) {
        redisFree(redis_context_);
        redis_context_ = NULL;
    }
} 

void RedisConnectionImpl::Done() {
    cLog(DEBUG, "Reclaim connection, id=%d", this->db_);
    Update(NULL, true, STATE_ERROR_INVOKE, NULL); 
    if (pool_) {
        pool_->Put(this);
    } else {
        BUG("RedisConnectionImpl manager do not exist");
    }
    return;
}

void RedisConnectionImpl::UpdateProcessState(redisReply* reply, 
        bool reclaim, 
        ERR_STATE state, 
        const char* err_msg, 
        bool copy_errstr) {
    reply_ptr_->Update(reply, reclaim, state, err_msg, copy_errstr);
}

bool RedisConnectionImpl::Connect(const std::string& host, int port, struct timeval &timeout, const std::string& password) {
    if (redis_context_) {
        this->Update(NULL, true, STATE_ERROR_INVOKE, ERR_REENTERING);
        cLog(ERROR, "internal implementation bug, redis_context is not NULL in 'Connect'");
        return false;
    }
    redis_context_ = redisConnectWithTimeout(host.c_str(), port, timeout);
    if (!redis_context_) {
        this->Update(NULL, true, STATE_ERROR_INVOKE, ERR_NO_CONTEXT);
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
    return this->ok();
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
        this->Update(NULL, true, STATE_ERROR_INVOKE, ERR_NO_CONTEXT);
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

// special for 'select' command
bool RedisConnectionImpl::Select(int db) {
    ++this->action_count_;
    this->Do("SELECT %d", db);
    bool ok = this->ok();
    if (ok) {
        this->db_ = db;
    }
    return ok;
}

RedisConnection::~RedisConnection() {
    cLog(DEBUG, "RedisConnection destructor...");
    if (impl_) {
        impl_->Done();
    }
}

RedisConnectionImpl* RedisConnection::operator->() {
    return this->impl_;
}

RedisConnectionImpl& RedisConnection::operator*() {
    return *(this->impl_);
}

RedisConnection& RedisConnection::operator=(RedisConnectionImpl* connection) {
    if (this->impl_) {
        this->impl_->Done();
    }
    this->impl_ = connection;
    return *this;
}

bool RedisConnection::operator!() {
    return this->impl_ ? true : false;
}

} // namespace cloris

