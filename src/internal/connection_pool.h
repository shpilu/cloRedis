// 
// Generic connection pool implementation
// with characteristic of both connection pool and memory pool, and thread-safe 
// Copyright (c) 2017 James Wei (weijianlhp@163.com). All rights reserved.
//

#ifndef CLORIS_CONNECTION_POOL_H_
#define CLORIS_CONNECTION_POOL_H_

#include<sys/time.h>
#include <unistd.h>
#include <mutex>
#include <forward_list>
#include <functional>

#define NUMBER_UNLIMITED -1

namespace cloris {

static uint64_t __get_current_time_ms() {
    struct timeval tv; 
    if (gettimeofday(&tv, NULL)) {   
        return 0;
    }
    return tv.tv_sec * 1000 + tv.tv_usec / 1000 ;
}

template <typename T>
struct ListItem {
    ListItem() 
        : active_time(__get_current_time_ms()),
          create_time(__get_current_time_ms()),
          next(NULL),
          prev(NULL) { 
    }
    ~ListItem() { }
    T* GetObject() { return reinterpret_cast<T*>(this + 1); }

    int64_t active_time; 
    int64_t create_time;
    ListItem<T> *next;
    ListItem<T> *prev;
};

template <typename T>
struct IdleList {
    IdleList() 
        : count(0), 
          front(NULL), 
          back(NULL) { 
    }
    ~IdleList() {
        for (ListItem<T> *tobj = front; tobj != NULL; tobj = tobj->next) {
            T* t = tobj->GetObject();
            t->~T();
            tobj->~ListItem<T>();
            free(tobj);
        }
    }
    void PushFront(ListItem<T> *item);
    void PopFront();
    void PopBack();

    int count;
    ListItem<T> *front;
    ListItem<T> *back;
};

template <typename T> 
void IdleList<T>::PushFront(ListItem<T> *item) {
    item->next = this->front;
    item->prev = NULL;
    if (this->count == 0) {
        this->back = item;
    } else {
        this->front->prev = item;
    }
    this->front = item;
    ++this->count;
    return;
}

template <typename T>
void IdleList<T>::PopFront() {
    if (this->count == 0) {
        return;
    }
    --this->count;
    ListItem<T> *item = this->front;
    if (this->count == 0) {
        this->front = NULL;
        this->back = NULL;
    } else {
        item->next->prev = NULL;
        this->front = item->next;
    }
    item->next = NULL;
    item->prev = NULL;
    return;
}

template <typename T>
void IdleList<T>::PopBack() {
    if (this->count == 0) {
        return;
    }
    --this->count;
    ListItem<T> *item = this->back;
    if (this->count == 0) {
        this->front = NULL;
        this->back = NULL;
    } else {
        item->prev->next = NULL;
        this->back = item->prev;
    }
    item->next = NULL;
    item->prev = NULL;
    return;
}

struct ConnectionPoolOption {
    ConnectionPoolOption() 
        : max_idle(NUMBER_UNLIMITED),
          max_active(NUMBER_UNLIMITED),
          idle_timeout_ms(NUMBER_UNLIMITED),
          max_conn_life_time(NUMBER_UNLIMITED) {
      }

    int max_idle;
    // Maximum number of connections allocated by the pool at a given time
    int max_active;
    int64_t idle_timeout_ms;
    int64_t max_conn_life_time;
};

struct ConnectionPoolStats {
    ConnectionPoolStats() : malloc_fail_(0), overload_error(0) { }

    int malloc_fail_;
    int overload_error;
};


// class 'Type' must implement 'ok' method
template <typename Type> 
class ConnectionPool {
public:
    typedef std::function<bool(void*)> InitHandler;

    ConnectionPool(InitHandler ihandler = NULL) : init_handler_(ihandler), active_cnt_(0) { }
    ConnectionPool(const ConnectionPoolOption* option, InitHandler ihandler = NULL) 
        : init_handler_(ihandler), 
          active_cnt_(0) { 
        if (option) {
            option_ = *option;
        }
    }
    ~ConnectionPool(); 
    Type* Get(std::string* err_msg = NULL);
    void Put(Type* type);

    int conn_in_pool() const { return idle_.count; }
    int active_cnt() const {  return active_cnt_; }

private:
    Type* GetNewInstance();
    void Gc(Type* type);

    InitHandler init_handler_;
    ConnectionPoolOption option_;
    ConnectionPoolStats stats_;
    IdleList<Type> idle_;
    std::forward_list<ListItem<Type>*> mem_pool_;
    // Number of connections allocated by the pool at a given time.
    int active_cnt_;
    std::mutex mutex_;
    std::mutex pool_mtx_;
};

template<typename Type>
ConnectionPool<Type>::~ConnectionPool() {
    ListItem<Type> *ptr(NULL); 
    while (!mem_pool_.empty()) {
        ptr = mem_pool_.front();
        mem_pool_.pop_front();
        free(ptr);
    }
}

template<typename Type>
Type* ConnectionPool<Type>::Get(std::string* err_msg) {
    //TODO 
    (void)err_msg;
    if (option_.max_active > 0 && active_cnt_ >= option_.max_active) {
        ++stats_.overload_error;
        return NULL;
    }
    std::unique_lock<std::mutex> lck(mutex_, std::defer_lock);
    if (option_.idle_timeout_ms > 0) {
        lck.lock();
        for (ListItem<Type> *item = idle_.back; 
            (item != NULL) && (item->active_time + option_.idle_timeout_ms < __get_current_time_ms()); 
            item = idle_.back) {
             idle_.PopBack();
             lck.unlock();
             Gc(item->GetObject());
             lck.lock();
        }
        lck.unlock();
    }
    lck.lock();
    for (ListItem<Type> *item = idle_.front; item != NULL; item = idle_.front) {
        idle_.PopFront();
        if ((option_.max_conn_life_time <= 0) || (__get_current_time_ms() - item->create_time < option_.max_conn_life_time)) {
            // TODO
            lck.unlock();
            return item->GetObject(); 
        }
        lck.unlock();
        Gc(item->GetObject());
        lck.lock();
    }
    lck.unlock();
    return GetNewInstance();
}

template<typename Type>
Type* ConnectionPool<Type>::GetNewInstance() {
    ListItem<Type>* ptr(NULL);
    {
        std::lock_guard<std::mutex> lk(pool_mtx_);
        if (!mem_pool_.empty()) {
            ptr = mem_pool_.front();
            mem_pool_.pop_front();
        }
    }
    if (!ptr) {
        ptr = (ListItem<Type>*)malloc(sizeof(ListItem<Type>) + sizeof(Type));
        if (!ptr) {
            ++stats_.malloc_fail_;
            return NULL;
        }
    }
    __sync_fetch_and_add(&active_cnt_, 1);
    // placement new
    new(ptr) ListItem<Type>;
    Type* obj_ptr = reinterpret_cast<Type*>(ptr + 1);
    new(obj_ptr) Type(this);
    if (init_handler_) {
        if (!init_handler_(obj_ptr)) {
            Gc(obj_ptr);
            return NULL;
        }
    }
    return obj_ptr;
}

template<typename Type>
void ConnectionPool<Type>::Gc(Type* type) {
    type->~Type();
    ListItem<Type> *item = reinterpret_cast<ListItem<Type>*>(type) - 1;
    item->~ListItem<Type>();
    {
        std::lock_guard<std::mutex> lk(pool_mtx_);
        mem_pool_.push_front(item);
    }
    __sync_fetch_and_sub(&active_cnt_, 1);
}

template<typename Type>
void ConnectionPool<Type>::Put(Type* type) {
    if (!type->ok()) {
        Gc(type);
        return;
    }

    ListItem<Type> *item = reinterpret_cast<ListItem<Type>*>(type) - 1;
    item->active_time = __get_current_time_ms();
    ListItem<Type> *idl(NULL);
    {
        std::lock_guard<std::mutex> lk(mutex_);
        idle_.PushFront(item);
        if ((option_.max_idle > 0) && (idle_.count > option_.max_idle)) {
            idl = idle_.back;
            idle_.PopBack(); 
        }
    }
    if (idl) {
        Gc(idl->GetObject());
    }
}

} // namespace cloris

#endif // CLORIS_CONNECTION_POOL_H_
