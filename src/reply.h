//
// cloRedis reply class definition
// RedisReply is an encapsulation of redisReply struct in hiredis
// version: 1.0 
// Copyright (C) 2018 James Wei (weijianlhp@163.com). All rights reserved
//

#ifndef CLORIS_REDIS_REPLY_H_
#define CLORIS_REDIS_REPLY_H_

#define REDIS_ERRSTR_LEN 256

#include "hiredis/hiredis.h"

namespace cloris {

enum ERR_STATE {
    STATE_OK = 0,
    STATE_ERROR_TYPE = 1,
    STATE_ERROR_COMMAND = 2,
    STATE_ERROR_HIREDIS = 3,
    STATE_ERROR_INVOKE = 4,
};

class RedisReply {
public:
    RedisReply();
    RedisReply(redisReply* reply, bool reclaim, ERR_STATE state, const char* err_msg);
    virtual ~RedisReply();

    bool error() const; 
    bool ok() const;
    std::string toString() const;
    int32_t toInt32() const;
    int64_t toInt64() const;
    int type() const;
    bool is_nil() const;
    bool is_string() const;
    bool is_int() const;
    bool is_array() const;
    size_t size() const;

    RedisReply get(size_t index);
    RedisReply operator[](size_t index);


    RedisReply(RedisReply&&); 
    RedisReply& operator=(RedisReply&&);

    std::string err_str() const;
    const char* err_msg() const { return err_msg_; }
    redisReply* mutable_reply() { return reply_; }
    ERR_STATE err_state() const { return err_state_; }
    bool reclaim() const { return reclaim_; }

protected:
    void Update(redisReply* rep, bool reclaim, ERR_STATE state, const char* err_msg); 
    redisReply* reply_;
    ERR_STATE err_state_;
private:
    RedisReply(const RedisReply&) = delete; 
    RedisReply& operator=(const RedisReply&) = delete;
    void UpdateErrMsg(const char* err_msg); 
    void Init(redisReply* rep, bool reclaim, ERR_STATE state, const char* err_msg); 
    void RemoveOldState();

    char err_msg_[REDIS_ERRSTR_LEN]; //
    bool reclaim_;
};

} // namespace cloris

#endif // CLORIS_REDIS_REPLY_H_
