// Cloredis core implementation
// Copyright (c) 2018 James Wei (weijianlhp@163.com). All rights reserved.
//
// ChangeLog:
// 2018-01-20  Created

#include <boost/algorithm/string.hpp>
#include <sstream>
#include <hiredis.h>
#include "public/singleton.h"
#include "public/log.h"
#include "cloredis.h"

namespace cloris {

static uint64_t get_current_time_ms() {
    struct timeval tval;
    if (gettimeofday(&tval, NULL)) {
        return 0;
    }
    return tval.tv_sec * 1000 + tval.tv_usec / 1000;
}

RedisConnectionImpl::RedisConnectionImpl(RedisManager* manager) 
    : redis_context_(NULL),
      manager_(manager),
      access_time_(get_current_time_ms()),
      action_count_(0),
      db_(0) {
}

RedisConnectionImpl::~RedisConnectionImpl() {
    Close();
}

void RedisConnectionImpl::Close() {
    if (redis_context_) {
        redisFree(redis_context_);
        redis_context_ = NULL;
    }
}

bool RedisConnectionImpl::Ok() const {
    return (state_.es == STATE_OK);
}

bool RedisConnectionImpl::ReclaimOk(int action_limit) {
    if (state_.es != STATE_OK) {
        cLog(DEBUG, "Connection reclaim failed as state not OK");
        return false;
    }
    if (db_ >= SLOT_NUM) {
        cLog(DEBUG, "Connection reclaim failed as db exceeding");
        return false;
    }
    if (this->action_count_ >= action_limit) {
        cLog(DEBUG, "Connection reclaim failed as action count exceeding, action_count=%d", action_count_);
        return false;
    }
    return true;
}

ERR_STATE RedisConnectionImpl::ErrorCode() const {
    return state_.es;
}

const std::string& RedisConnectionImpl::ErrorString() const {
    return state_.msg;
}

void RedisConnectionImpl::Done() {
    cLog(DEBUG, "Reclaim connection, id=%d", this->db_);
    access_time_ = get_current_time_ms();
    if (manager_) {
        manager_->Gc(this);
    } else {
        cLog(ERROR, "RedisConnectionImpl manager do not exist");
    }
    return;
}

void RedisConnectionImpl::UpdateProcessState(ERR_STATE state, const std::string& msg) {
    state_.es = state;
    state_.msg = msg;
}

bool RedisConnectionImpl::Connect(const std::string& host, int port, struct timeval &timeout, const std::string& password) {
    if (redis_context_) {
        this->UpdateProcessState(STATE_ERROR_INVOKE, ERR_REENTERING);
        cLog(ERROR, "internal implementation bug, redis_context is not NULL in 'Connect'");
        return false;
    }
    redis_context_ = redisConnectWithTimeout(host.c_str(), port, timeout);
    if (!redis_context_) {
        this->UpdateProcessState(STATE_ERROR_INVOKE, ERR_NO_MEMORY);
        cLog(ERROR, "No memory in creating new request");
        return false;
    }
    if (redis_context_->err) {
        this->UpdateProcessState(STATE_ERROR_HIREDIS, redis_context_->errstr);
        return false;
    }
    if (password.size() == 0) {
        this->UpdateProcessState(STATE_OK, "");
        return true;
    }
    this->Do("AUTH %s", password.c_str());
    return this->Ok();
}

std::string RedisConnectionImpl::Do(const char *format, ...) {
    ++this->action_count_;
    std::string value("");
    if (!redis_context_) {
        this->UpdateProcessState(STATE_ERROR_INVOKE, ERR_NOT_INITED);
        return value;
    }

    va_list ap;
    va_start(ap, format);
    redisReply* reply = (redisReply*)redisvCommand(redis_context_, format, ap);
    va_end(ap);

    if (redis_context_->err) {
        this->UpdateProcessState(STATE_ERROR_HIREDIS, redis_context_->errstr);
        return value;
    }

    if (reply != NULL) {
        if (reply->type == REDIS_REPLY_STRING) {
            value = reply->str;
            this->UpdateProcessState(STATE_OK, "");
        } else if (reply->type == REDIS_REPLY_ERROR) {
            this->UpdateProcessState(STATE_ERROR_COMMAND, reply->str);
        } else {
            this->UpdateProcessState(STATE_OK, "");
        }
        freeReplyObject(reply);
    } else {
        this->UpdateProcessState(STATE_ERROR_INVOKE, ERR_NO_REPLY);
    }

    return value;
}

bool RedisConnectionImpl::Select(int db) {
    ++this->action_count_;
    this->Do("SELECT %d", db);
    bool ok = this->Ok();
    if (ok) {
        this->db_ = db;
    }
    return ok;
}

RedisConnection::~RedisConnection() {
    cLog(DEBUG, "RedisConnection destructor...");
    if (impl_)  {
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

RedisManager::RedisManager() 
    : host_("127.0.0.1"),
      port_(6379),
      timeout_({1, 0}),
      password_(""),
      action_limit_(50),
      time_limit_(60000) {
    cLog(TRACE, "RedisManager constructor "); 
}

RedisManager::~RedisManager() {
    cLog(TRACE, "RedisManager ~ destructor ");
}

RedisManager* RedisManager::instance() {
    return Singleton<RedisManager>::instance();
}

// TODO support config
RedisManager::RedisManager(const std::string& host, int port, int timeout_ms, const std::string& password) 
    : host_(host),
      port_(port),
      timeout_({timeout_ms / 1000, (timeout_ms % 1000) * 1000}),
      password_(password),
      action_limit_(50),
      time_limit_(60000) {
}

bool RedisManager::Connect(const std::string& host, int port, int timeout_ms, const std::string& password) {
    host_ = host;
    port_ = port;
    timeout_ = { timeout_ms / 1000, (timeout_ms % 1000) * 1000 };
    password_ = password;
    
    RedisConnectionImpl *connection = new RedisConnectionImpl(this);
    if (connection->Connect(host_, port, timeout_, password_)) {
        this->Gc(connection);
        return true;
    } else {
        return false;
    }
}

RedisConnectionImpl* RedisManager::Get(int db) {
    RedisConnectionImpl* conn = NULL;
    ConnectionQueue* cqueue = &queue_[db];
    if (cqueue->conns.size() > 0) {
        uint64_t now = get_current_time_ms();
        std::lock_guard<std::mutex> lock(cqueue->queue_mtx);
        if (cqueue->conns.size() > 0) {
            RedisConnectionImpl* tmp_conn = cqueue->conns.front();
            if (now - tmp_conn->access_time_ > this->time_limit_) {
                cqueue->conns.pop_front();
                cLog(DEBUG, "delete connection as time_limit exceed, slot=%d", db);
                delete tmp_conn;
            } else {
                conn = tmp_conn; 
                cqueue->conns.pop_front();
                cLog(DEBUG, "Get connection from pool, slot=%d", db);
            }
        }
    }
    
    // TODO get_connection from one slot with most count 
    if (!conn) {
    }

    // new一个
    if (!conn) {
        cLog(INFO, "Get connection by creating new one, slot=%d", db);
        conn = new RedisConnectionImpl(this);
        conn->Connect(host_, port_, timeout_, password_);
        conn->Select(db);
    }
    return conn;
}

void RedisManager::Gc(RedisConnectionImpl* connection) {
    if (!connection) {
        return;
    }
    if (!connection->ReclaimOk(this->action_limit_)) {
        delete connection;
        return;
    }
    ConnectionQueue* cqueue = &queue_[connection->db()];
    {
        std::lock_guard<std::mutex> lock(cqueue->queue_mtx);
        // LRU queue
        cqueue->conns.push_front(connection);
    }
    cLog(INFO, "Put connection back into connection pool, slot=%d", connection->db());
}

} // namespace cloris
