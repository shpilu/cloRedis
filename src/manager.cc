#include "redis_manager.h"

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
      slave_cnt_(0),
      master_(NULL),
      slave_(NULL) {
    cLog(TRACE, "RedisManager constructor ");
}

RedisManager::~RedisManager() {
    this->Flush();
    cLog(TRACE, "RedisManager ~ destructor");
}

void RedisManager::Flush() {
    if (master_) {
        delete master_;
    }
    if (slave_) {
        for (int i = 0; i < slave_cnt_; ++i) {
            delete slave_[i];
        }
        free(slave_);
    }
    inited_ = false;
}

RedisManager* RedisManager::instance() {
    return Singleton<RedisManager>::instance();
}

bool RedisManager::Connect(const std::string& host, 
             const std::string& password = "", 
             int timeout_ms = DEFAULT_TIMEOUT_MS, 
             ConnectionOption* option = NULL, 
             std::string *err_msg = NULL) {
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
    BUG_ON(master_ != NULL);
    master_ = new RedisConnectionPool();
    if (!master_->Init(address_vec[0].host, address_vec[0].port, timeout_ms, option, err_msg)) {
        return false;
    } else {
        return true;
    }
}

bool RedisManager::ConnectEx(const std::string& master_host, 
               const std::string& slave_hosts, 
               const std::string& password = "", 
               int timeout_ms = DEFAULT_TIMEOUT_MS,
               ConnectionOption* option = NULL,
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
    BUG_ON(master_ != NULL);
    master_ = new RedisConnectionPool();
    if (!master_->Init(master_address_vec[0].host, master_address_vec[0].port, timeout_ms, option, err_msg)) {
        return false;
    } 
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

RedisConnectionImpl* RedisManager::Get(int db, RedisRole role = MASTER, int index = -1) {
    if (role == MASTER) {
        if (master_) {
            return master_->Get(db);
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
}

} // namespace cloris
