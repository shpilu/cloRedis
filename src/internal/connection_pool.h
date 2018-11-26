// 
// General connection pool implementation
// To some extent inspired by redigo (https://github.com/gomodule/redigo)
// Copyright 2017 James Wei (weijianlhp@163.com)
//

#ifndef CLORIS_CONNECTION_POOL_H_
#define CLORIS_CONNECTION_POOL_H_

namespace cloris {

template <typename T>
struct ListItem {
    T* obj;
    int64_t active_time; 
    int64_t create_time;
    ListItem<T> *next;
    ListItem<T> *prev;
};

template <typename T>
struct IdleList {
    void PushFront(ListItem<T> *item);
    void PopFront();
    void PopBack();

    int count;
    ListItem<T> *front;
    ListItem<T> *back;
};

template <typename T> 
void IdleList<T>::PushFront(ListItem<T> *item) {
    item->next = this->next;
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
        item->next.prev = NULL;
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

class GenericConnection {
public:
    virtual void GenericConnection() {}
    virtual void ~GenericConnection() {}
private:
};

template <class GenericType, class Type> 
class ConnectionPool {
public:
   void ConnectionPool(); 
   
   void Connect();
   void TestOnBorrow();
   Type* Get(int partition = 0);
   void Put(Type* type);
   void Close();

private:
    int max_idle_;
    int max_active_;
    int64_t idle_timeout_ms_:
    int64_t max_conn_life_time_;
    bool closed_;
    int  active_cnt_:
    int  partition_cnt_;
    IdleList<Type>* idle_;
};

template<class GenericType, class Type>
ConnectionPool<GenericType, Type>::ConnectionPool() {
   // check if Type is child class of GenericType in compile time
   GenericType* conn = (Type*)NULL;
   (void)conn;
}





} // namespace cloris

#endif // CLORIS_CONNECTION_POOL_H_
