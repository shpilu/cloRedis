#ifndef CLORIS_REDIS_REPLY_H_
#define CLORIS_REDIS_REPLY_H_

#define REDIS_ERRSTR_LEN 256

#include "hiredis/hiredis.h"

namespace cloris {

class RedisReply;
typedef std::shared_ptr<RedisReply> RedisReplyPtr;

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
    ~RedisReply();

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
protected:
    void Update(redisReply* rep, bool reclaim, ERR_STATE state, const char* err_msg); 
private:
    void UpdateErrMsg(const char* err_msg); 
    char err_str_[REDIS_ERRSTR_LEN]; //
    redisReply* reply_;
    bool reclaim_;
    ERR_STATE err_state_;
};

} // namespace cloris

#endif // CLORIS_REDIS_REPLY_H_
