
#include <vector>
#include <boost/algorithm/string.hpp>
#include "internal/singleton.h"
#include "internal/log.h"
#include "cloredis.h"

namespace cloris {

static std::vector<ServiceAddress> parse_address_vector(const std::string& host) {
    std::vector<ServiceAddress> addr_vec;
    std::vector<std::string> vec_raw;
    boost::split(vec_raw, host, boost::is_any_of(","));
    for (auto &full_host : vec_raw) {
        std::vector<std::string> addr_pair;
        boost::split(addr_pair, full_host, boost::is_any_of(":"));
        if (addr_pair.size() == 2) {
            int port = atoi(addr_pair[1].c_str());
            if (port > 0 && port < 65536) {
                ServiceAddress addr;
                addr.port = port;
                addr.host = addr_pair[0];
                addr.full_host = full_host;
                boost::trim(addr.host);
                boost::trim(addr.full_host);
                addr_vec.push_back(addr);
            }
        }
    }
    return addr_vec;
}

RedisManager::RedisManager() 
    : inited_(false),
      slave_cnt_(0) {
    cLog(TRACE, "RedisManager constructor ");
    memset(master_, 0, sizeof(RedisConnectionPool*) * MAX_DB_NUM);
}

RedisManager::~RedisManager() {
    this->Flush();
    cLog(TRACE, "RedisManager ~ destructor");
}

void RedisManager::Flush() {
    for (int i = 0; i < MAX_DB_NUM; ++i) {
        if (master_[i]) {
            delete master_[i];
        }
        /*
        if (slave_[i]) {
            for (int j = 0; j < slave_cnt_; ++j) {
                delete slave_[i][j];
            }
            free(slave_[i]);
        }
        */
    }
    inited_ = false;
}

RedisManager* RedisManager::instance() {
    return Singleton<RedisManager>::instance();
}

bool RedisManager::Init(const std::string& host, 
             const std::string& password, 
             int timeout_ms, 
             ConnectionPoolOption* option, 
             std::string *err_msg) {
    if (inited_) {
        if (err_msg) {
            *err_msg = ERR_REENTERING;
        }
        return false;
    }
    inited_ = true;
    std::vector<ServiceAddress> address_vec = parse_address_vector(host);
    if (address_vec.size() != 1) {
        if (err_msg) {
            *err_msg = ERR_BAD_HOST;
        }
        return false;
    }
    RedisConnectionPool::InitHandler handler = std::bind(&RedisConnectionImpl::Init, std::placeholders::_1, 
            address_vec[0].host, 
            address_vec[0].port, 
            password,
            timeout_ms);
    master_[DEFAULT_DB] = new RedisConnectionPool(option, handler);

    RedisConnection conn = master_[DEFAULT_DB]->Get(err_msg);

    return conn ? true : false;
}

/*
bool RedisManager::InitEx(const std::string& master_host, 
               const std::string& slave_hosts, 
               const std::string& password = "", 
               int timeout_ms = DEFAULT_TIMEOUT_MS,
               ConnectionPoolOption* option = NULL,
               std::string* err_msg = NULL) {
    if (inited_) {
        if (err_msg) {
            *err_msg = ERR_REENTERING;
        }
        return false;
    }
    inited_ = true;
    std::vector<ServiceAddress> master_address_vec = parse_address_vector(master_host);
    if (master_address_vec.size() != 1) {
        if (err_msg) {
            *err_msg = ERR_BAD_HOST;
        }
        return false;
    }
    InitHandler handler = std::bind(&RedisConnectionImpl::Connect, std::placeholders::_1, 
            master_address_vec[0].host, 
            master_address_vec[0].port, 
            timeout_ms);

    master_[0] = new RedisConnectionPool(handler);

    std::vector<RedisAddress> slave_address_vec = parse_address_vector(slave_hosts);
    if (slave_address_vec.size() < 1) {
        return true;
    }
    slave_ = (RedisConnectionPool**)malloc(sizeof(RedisConnectionPool*) * slave_address_vec.size());
    if (!slave_) {
        return false;
    }
    for (int i = 0; i < slave_address_vec.size(); ++i) {
        slave_[i] = new RedisConnectionPool();
        ++slave_cnt_;
        if (!slave_[i]->Init(slave_address_vec[i].host, slave_address_vec[i].port, timeout_ms, option, err_msg)) {
            return false;
        }
    }
    return true;
}
*/

RedisConnectionImpl* RedisManager::Get(int db, std::string* err_msg, RedisRole role, int index) {
    // TODO delete it
    (void)role;
    (void)index;
    if (db >= MAX_DB_NUM) {
        return NULL;
    }
    if (master_[db]) {
        return master_[db]->Get(err_msg);
    } else {
        return NULL;
    }

    /*
    if (role == MASTER) {
        if (master_[db]) {
            return master_[db]->Get();
        } else {
            return NULL;
        }
    } else {
        if (slave_ ) {
            int real_index = (index >= 0 && index < slave_cnt_) ? index : (rand() % slave_cnt_);
            return slave_[real_index]->Get(db);
        } else {
            return NULL;
        }
    }
    */
}

} // namespace cloris
