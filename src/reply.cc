#include <memory>
#include "hiredis/hiredis.h"
#include "internal/log.h"
#include "reply.h"

namespace cloris {

RedisReply::RedisReply() {
    cLog(TRACE, "RedisReply constructor..."); 
    Update(NULL, true, STATE_ERROR_INVOKE, NULL); 
}

RedisReply::RedisReply(redisReply* reply, bool reclaim, ERR_STATE state, const char* err_msg) {
    cLog(TRACE, "RedisReply constructor..."); 
    Update(reply, reclaim, state, err_msg);
}

RedisReply::~RedisReply() {
    cLog(TRACE, "RedisReply destructor..."); 
    if (reclaim_ && reply_) {
        freeReplyObject(reply_);
    }
}

void RedisReply::UpdateErrMsg(const char* err_msg) {
    size_t len = strlen(err_msg);
    len = (len < sizeof(err_str_)) ? len : (sizeof(err_str_) - 1);
    memcpy(err_str_, err_msg, len);
    err_str_[len + 1] = '\0';
}

void RedisReply::Update(redisReply* rep, bool reclaim, ERR_STATE state, const char* err_msg) {
    if (reply_) {
        freeReplyObject(reply_);
    }
    reply_ = rep;
    reclaim_ = reclaim;
    err_state_ = state;
    UpdateErrMsg(err_msg);
}

std::string RedisReply::toString() const {
    std::string value("");
    if (reply_) {
        switch (reply_->type) {
            case REDIS_REPLY_STRING:
            case REDIS_REPLY_STATUS:
                value = reply_->str;
                break;
            case REDIS_REPLY_ERROR:
                if (reply_->err) {
                    value = reply_->str;
                }
                break;
            default:
                ;
        }
    }
    return value;
}

int32_t RedisReply::toInt32() const {
    int32_t value(0);
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
        value = this->err_str_;
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
        reply.reset(new RedisReply(reply_->element[index], false, STATE_OK, ""));
    }
    return reply;
}

} // namespace cloris

