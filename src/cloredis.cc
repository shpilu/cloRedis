//
// Copyright (c) 2018 James Wei (weijianlhp@163.com). All rights reserved.
// Cloredis core implementation
//
// ChangeLog:
// 2018-04-20  Created

#include <boost/algorithm/string.hpp>
#include <sstream>
#include "hiredis/hiredis.h"
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

RedisReply::RedisReply(redisReply* reply, bool reclaim, ERR_STATE state, const char* err_msg, bool copy_errstr)
    : reply_(reply),
      err_state_(state),
      reclaim_(reclaim) {
    UpdateErrMsg(err_msg, copy_errstr);
    cLog(TRACE, "RedisReply constructor..."); 
}

RedisReply::~RedisReply() {
    cLog(TRACE, "RedisReply destructor..."); 
    if (reclaim_ && reply_) {
        freeReplyObject(reply_);
    }
}

void RedisReply::UpdateErrMsg(const char* err_msg, bool copy_errstr) {
    if (copy_errstr) {
        size_t len = strlen(err_msg);
        len = (len < sizeof(err_str_)) ? len : (sizeof(err_str_) - 1);
        memcpy(err_str_, err_msg, len);
        err_msg_ = err_str_;
    } else {
        err_msg_ = err_msg;
    }
}

void RedisReply::Update(redisReply* rep, bool reclaim, ERR_STATE state, const char* err_msg, bool copy_errstr) {
    if (reply_) {
        freeReplyObject(reply_);
    }
    reply_ = rep;
    err_state_ = state;
    reclaim_ = reclaim;
    UpdateErrMsg(err_msg, copy_errstr);
}

std::string RedisReply::toString() const {
    std::string value("");
    if (reply_) {
        switch (reply_->type) {
            case REDIS_REPLY_STRING:
            case REDIS_REPLY_STATUS:
            case REDIS_REPLY_ERROR:
                value = reply_->str;
                break;
            default:
                ;
        }
    }
    return value;
}

int32_t RedisReply::toInt32() const {
    int32_t value(-1);
    if (reply_) {
        if (reply_->type == REDIS_REPLY_INTEGER) {
            value = reply_->integer;
        }
    }
    return value;
}

int64_t RedisReply::toInt64() const {
    int64_t value(-1);
    if (reply_) {
        if (reply_->type == REDIS_REPLY_INTEGER) {
            value = reply_->integer;
        }
    }
    return value;
}


bool RedisReply::error() const {  
    return (!reply_ || (reply_->type == REDIS_REPLY_ERROR));
}

bool RedisReply::ok() const {  
    return (reply_ && (reply_->type != REDIS_REPLY_ERROR)); 
}

std::string RedisReply::err_str() const {
    std::string value("");
    if (reply_) {
        if (reply_->type == REDIS_REPLY_ERROR) {
            value = reply_->str;
        }
    } else {
        value = this->err_msg_;
    }
    return value;
}

int RedisReply::type() const {
    if (reply_) {
        return reply_->type;
    } else {
        return REDIS_REPLY_ERROR;
    }
}

bool RedisReply::is_nil() const {
    return (reply_ && (reply_->type == REDIS_REPLY_NIL));
}

bool RedisReply::is_string() const {
    return (reply_ && (reply_->type == REDIS_REPLY_STRING));
}

bool RedisReply::is_int() const {
    return (reply_ && (reply_->type == REDIS_REPLY_INTEGER));
}

bool RedisReply::is_array() const {
    return (reply_ && (reply_->type == REDIS_REPLY_ARRAY));
}

size_t RedisReply::size() const {
    if (reply_ && (reply_->type == REDIS_REPLY_ARRAY)) {
        return reply_->elements;
    } else {
        return 0;
    }
}

RedisReplyPtr RedisReply::operator[](size_t index) {
    RedisReplyPtr reply;
    if (!reply_ || (reply_->type != REDIS_REPLY_ARRAY) || (index > reply_->elements)) {
        cLog(ERROR, "warning, reply type is not array");
    } else {
        reply.reset(new RedisReply(reply_->element[index], false, STATE_OK, "", false));
    }
    return reply;
}


ERR_STATE RedisConnectionImpl::err_state() const {
    return this->reply_ptr_->err_state();
}

bool RedisConnectionImpl::error() const {
    return this->reply_ptr_->error();
}

bool RedisConnectionImpl::ok() const {
    return this->reply_ptr_->ok();
}
    
std::string RedisConnectionImpl::toString() const {
    return this->reply_ptr_->toString();
}

int32_t RedisConnectionImpl::toInt32() const {
    return this->reply_ptr_->toInt32();
}

int64_t RedisConnectionImpl::toInt64() const {
    return this->reply_ptr_->toInt64();
}

std::string RedisConnectionImpl::err_str() const {
    return this->reply_ptr_->err_str();
}

int RedisConnectionImpl::type() const {
    return this->reply_ptr_->type();
}

bool RedisConnectionImpl::is_nil() const {
    return this->reply_ptr_->is_nil();
}

bool RedisConnectionImpl::is_string() const {
    return this->reply_ptr_->is_string();
}
bool RedisConnectionImpl::is_int() const {
    return this->reply_ptr_->is_int();
}

bool RedisConnectionImpl::is_array() const {
    return this->reply_ptr_->is_array();
}

size_t RedisConnectionImpl::size() const {
    return this->reply_ptr_->size();
}

RedisReplyPtr RedisConnectionImpl::Reply() {
    return this->reply_ptr_;
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

bool RedisConnectionImpl::ReclaimOk(int action_limit) {
    if (reply_ptr_->err_state() != STATE_OK) {
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

void RedisConnectionImpl::Done() {
    cLog(DEBUG, "Reclaim connection, id=%d, reset.use_count=%d", this->db_, reply_ptr_.use_count());
    reply_ptr_.reset(new RedisReply(NULL, true, STATE_OK, "", false));
    access_time_ = get_current_time_ms();
    if (manager_) {
        manager_->Gc(this);
    } else {
        cLog(ERROR, "RedisConnectionImpl manager do not exist");
    }
    return;
}

void RedisConnectionImpl::UpdateProcessState(redisReply* reply, bool reclaim, ERR_STATE state, const char* err_msg, bool copy_errstr) {
    cLog(DEBUG, "reply_ptr use_count=%d", reply_ptr_.use_count());
    if (reply_ptr_.use_count() == 1) {
        reply_ptr_->Update(reply, reclaim, state, err_msg, copy_errstr);
    } else {
        reply_ptr_.reset(new RedisReply(reply, reclaim, state, err_msg, copy_errstr));
    }
    if (err_msg) {
        manager_->AddErrorRecord(this);
    }
}

bool RedisConnectionImpl::Connect(const std::string& host, int port, struct timeval &timeout, const std::string& password) {
    if (redis_context_) {
        this->UpdateProcessState(NULL, true, STATE_ERROR_INVOKE, ERR_REENTERING);
        cLog(ERROR, "internal implementation bug, redis_context is not NULL in 'Connect'");
        return false;
    }
    redis_context_ = redisConnectWithTimeout(host.c_str(), port, timeout);
    if (!redis_context_) {
        this->UpdateProcessState(NULL, true, STATE_ERROR_INVOKE, ERR_NO_CONTEXT);
        cLog(ERROR, "No memory in creating new request");
        return false;
    }
    if (redis_context_->err) {
        this->UpdateProcessState(NULL, true, STATE_ERROR_HIREDIS, redis_context_->errstr, true);
        return false;
    }
    if (password.size() == 0) {
        this->UpdateProcessState(NULL, true, STATE_OK, "");
        return true;
    }
    this->Do("AUTH %s", password.c_str());
    return this->reply_ptr_->ok();
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
        this->UpdateProcessState(NULL, true, STATE_ERROR_INVOKE, ERR_NO_CONTEXT);
        return *this; 
    }
    redisReply* reply = (redisReply*)redisvCommand(redis_context_, format, ap);
    if (redis_context_->err) {
        this->UpdateProcessState(NULL, true, STATE_ERROR_HIREDIS, redis_context_->errstr, true);
        return *this; 
    }

    if (reply != NULL) {
        this->UpdateProcessState(reply, true, STATE_OK, "");
    } else {
        this->UpdateProcessState(NULL, true, STATE_ERROR_INVOKE, ERR_REPLY_NULL);
    }
    return *this;
}

bool RedisConnectionImpl::Select(int db) {
    ++this->action_count_;
    this->Do("SELECT %d", db);
    bool ok = this->reply_ptr_->ok();
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
      time_limit_(60000),
      conn_in_use_(0),
      conn_in_pool_(0) {
    cLog(TRACE, "RedisManager constructor "); 
}

RedisManager::~RedisManager() {
    cLog(TRACE, "RedisManager ~ destructor begin...");
    RedisConnectionImpl* tmp_conn;
    for (int i = 0; i < SLOT_NUM; ++i) {
        ConnectionQueue* cqueue = &queue_[i];
        while (!cqueue->conns.empty()) {
            tmp_conn = cqueue->conns.front();
            cqueue->conns.pop_front();
            delete tmp_conn;
        }
    }
    cLog(TRACE, "RedisManager ~ destructor end...");
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
      time_limit_(60000),
      conn_in_use_(0),
      conn_in_pool_(0) {
}


void RedisManager::AddErrorRecord(RedisConnectionImpl* conn) {
    err_str_ = conn->err_str();
}

bool RedisManager::Connect(const std::string& host, int port, int timeout_ms, const std::string& password) {
    host_ = host;
    port_ = port;
    timeout_ = { timeout_ms / 1000, (timeout_ms % 1000) * 1000 };
    password_ = password;
    
    RedisConnection conn = this->Get(0); 

    return conn->ok();
}

RedisConnectionImpl* RedisManager::Get(int db) {
    RedisConnectionImpl* conn = NULL;
    ConnectionQueue* cqueue = &queue_[db];
    if (!cqueue->conns.empty()) {
        uint64_t now = get_current_time_ms();
        std::lock_guard<std::mutex> lock(cqueue->queue_mtx);
        if (!cqueue->conns.empty()) {
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
            __sync_fetch_and_sub(&conn_in_pool_, 1);
        }
    }
    
    // TODO get_connection from one slot with most amount 
    if (!conn) {
    }

    if (!conn) {
        cLog(INFO, "Get connection by creating new one, slot=%d", db);
        conn = new RedisConnectionImpl(this);
        conn->Connect(host_, port_, timeout_, password_);
        conn->Select(db);
    }
    __sync_fetch_and_add(&conn_in_use_, 1);
    return conn;
}

// TODO has bug, release connnection
RedisConnectionImpl& RedisManager::Do(int db, const char *format, ...) {
    RedisConnectionImpl* conn_impl = this->Get(db); 
    va_list ap;
    va_start(ap, format);
    conn_impl->__Do(format, ap);
    va_end(ap);
    return *conn_impl;
}

void RedisManager::Gc(RedisConnectionImpl* connection) {
    __sync_fetch_and_sub(&conn_in_use_, 1);
    if (!connection->ReclaimOk(this->action_limit_)) {
        delete connection;
        return;
    }
    ConnectionQueue* cqueue = &queue_[connection->db()];
    {
        std::lock_guard<std::mutex> lock(cqueue->queue_mtx);
        // LRU queue
        cqueue->conns.push_front(connection);
        __sync_fetch_and_add(&conn_in_pool_, 1);
    }
    cLog(INFO, "Put connection back into connection pool, slot=%d", connection->db());
}

} // namespace cloris
