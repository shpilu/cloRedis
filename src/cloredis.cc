
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
    : password_(""),
      timeout_ms_(-1),
      inited_(false),
      slave_cnt_(0) {
    cLog(TRACE, "RedisManager constructor ");
    memset(master_, 0, sizeof(RedisConnectionPool*) * MAX_DB_NUM);
    memset(slave_, 0, sizeof(RedisConnectionPool**) * MAX_DB_NUM);
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
        if (slave_[i]) {
            for (int j = 0; j < slave_cnt_; ++j) {
                delete slave_[i][j];
            }
            free(slave_[i]);
        }
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
        cLog(ERROR, ERR_REENTERING);
        if (err_msg) {
            *err_msg = ERR_REENTERING;
        }
        return false;
    }
    inited_ = true;
    std::vector<ServiceAddress> address_vec = parse_address_vector(host);
    if (address_vec.size() != 1) {
        cLog(ERROR, ERR_BAD_HOST);
        if (err_msg) {
            *err_msg = ERR_BAD_HOST;
        }
        return false;
    }
    master_addr_ = address_vec[0];
    if (option) {
        option_ = *option;
    }
    password_ = password;
    timeout_ms_ = timeout_ms;

    RedisConnectionPool::InitHandler handler = std::bind(&RedisConnectionImpl::Init, std::placeholders::_1, 
            master_addr_.host, 
            master_addr_.port, 
            password_,
            timeout_ms_, DEFAULT_DB);
    master_[DEFAULT_DB] = new RedisConnectionPool(&option_, handler);

    RedisConnection conn = master_[DEFAULT_DB]->Get(err_msg);
    cLogIf(!conn, ERROR, err_msg ? err_msg->c_str() : "");

    return conn ? true : false;
}

bool RedisManager::InitEx(const std::string& master_host, 
               const std::string& slave_hosts, 
               const std::string& password, 
               int timeout_ms,
               ConnectionPoolOption* option,
               std::string* err_msg) {
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

    master_addr_ = master_address_vec[0];
    if (option) {
        option_ = *option;
    }
    password_ = password;
    timeout_ms_ = timeout_ms;

    // slave is optional
    std::vector<ServiceAddress> slave_address_vec = parse_address_vector(slave_hosts);
    if (slave_address_vec.size() > 0 && slave_address_vec.size() < MAX_SLAVE_CNT) {
        for (auto &p : slave_address_vec) {
            slave_addr_.push_back(p);
        }
    }
    slave_cnt_ = slave_addr_.size();

    int msize = sizeof(RedisConnectionPool*) * slave_addr_.size();
    slave_[DEFAULT_DB] = (RedisConnectionPool**)malloc(msize);
    memset(slave_[DEFAULT_DB], 0, msize); 
    if (!slave_) {
        if (err_msg) {
            *err_msg = "malloc failed";
        }
        return false;
    }

    RedisConnectionPool::InitHandler master_handler = std::bind(&RedisConnectionImpl::Init, std::placeholders::_1, 
            master_addr_.host, 
            master_addr_.port, 
            password_,
            timeout_ms_, DEFAULT_DB);
    master_[DEFAULT_DB] = new RedisConnectionPool(&option_, master_handler);

    RedisConnectionPool::InitHandler slave_handler = std::bind(&RedisConnectionImpl::Init, std::placeholders::_1, 
            slave_addr_[0].host, 
            slave_addr_[0].port, 
            password_,
            timeout_ms_, DEFAULT_DB);
    slave_[DEFAULT_DB][0] = new RedisConnectionPool(&option_, slave_handler);

    RedisConnection master_conn = master_[DEFAULT_DB]->Get(err_msg);
    RedisConnection slave_conn = slave_[DEFAULT_DB][0]->Get(err_msg);

    return (master_conn && slave_conn) ? true : false;
}

RedisConnectionImpl* RedisManager::Get(int db, std::string* err_msg, RedisRole role, int index) {
    // TODO delete it
    (void)role;
    (void)index;
    if (db >= MAX_DB_NUM) {
        return NULL;
    }
    
    // if no slave instance exists, role is ignored
    if ((role == MASTER) || (slave_cnt_ < 1)) {
        if (master_[db]) {
            return master_[db]->Get(err_msg);
        } else {
            RedisConnectionPool::InitHandler handler = std::bind(&RedisConnectionImpl::Init, std::placeholders::_1, 
                    master_addr_.host, 
                    master_addr_.port, 
                    password_,
                    timeout_ms_, 
                    db);
            master_[db] = new RedisConnectionPool(&option_, handler);
            return master_[db]->Get(err_msg);
        }
    } else {
        int real_index = (index >= 0 && index < slave_cnt_) ? index : (rand() % slave_cnt_);
        if (slave_[db][real_index]) {
            return slave_[db][real_index]->Get(err_msg);
        } else {
            RedisConnectionPool::InitHandler handler = std::bind(&RedisConnectionImpl::Init, std::placeholders::_1, 
                    slave_addr_[real_index].host, 
                    slave_addr_[real_index].port, 
                    password_,
                    timeout_ms_, 
                    db);
            slave_[db][real_index] = new RedisConnectionPool(&option_, handler);
            return slave_[db][real_index]->Get(err_msg);
        }
    }
}

int RedisManager::ActiveConnectionCount(RedisRole role) {
    int count = 0;
    if (role == MASTER) {
        for (int i = 0; i < MAX_DB_NUM; ++i) {
            if (master_[i]) {
                count += master_[i]->active_cnt();
            }
        }
    } else {
        for (int i = 0; i < MAX_DB_NUM; ++i) {
            for (int j = 0; j < slave_cnt_; ++j) {
                count += slave_[i][j]->active_cnt();
            }
        }
    }
    return count;
}

int RedisManager::ConnectionInPool(RedisRole role) {
    int count = 0;
    if (role == MASTER) {
        for (int i = 0; i < MAX_DB_NUM; ++i) {
            if (master_[i]) {
                count += master_[i]->conn_in_pool();
            }
        }
    } else {
        for (int i = 0; i < MAX_DB_NUM; ++i) {
            for (int j = 0; j < slave_cnt_; ++j) {
                count += slave_[i][j]->conn_in_pool();
            }
        }
    }
    return count;
}

int RedisManager::ConnectionInUse(RedisRole role) {
    return ActiveConnectionCount(role) - ConnectionInPool(role);
}

} // namespace cloris
