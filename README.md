cloRedis
=====

Cloredis is a simple, high-performance C++ Client for Redis with native support for connection pool and master/slave instances access 

The design of cloRedis references and integrates many leading Redis Client in C++/Golang language, e.g. [hiredis](https://github.com/redis/hiredis.git), [redigo](https://github.com/gomodule/redigo.git), [redis3m](https://github.com/luca3m/redis3m.git) and [brpc](https://github.com/brpc/brpc.git), and has its own features.

* [Features](#features)

## Features<div id="features"></div>

* Support connection pool and memory pool naturally -- All connection operations are based on connection pool.
* Automatic memory management and connection reclaim -- You do not need any memory operation, nor need you put the connection back to connection pool when it is no longer required.  
* Support master/slave replication -- You can choose access master or slave instance in redis replication environment.

Cloredis's features make it well adapted for production environment. Now cloredis is used in ofo Inc. and works very well.

## Usage

The following shows how to get a connection from connection pool, select specific db and run redis command by using redis connection
``` C++
RedisManager *manager = RedisManager::instance();
if (!manager->Init("172.17.224.212:6379", "cloris520", 100)) {
    std::cout << "init redis manager failed" << std::endl;
}
{
    // note: You do not need to release 'conn1' as cloredis will do it automatically
    RedisConnection conn1 = manager->Get(1); 
    conn1->Do("SET tkey1 %d", 100);
    std::cout << conn1->Do("GET tkey1").toString() << std::endl; // 100
}
...
```

## Installation

On Linux system you can build cloredis compile and runtime environment by running the following commands:
``` shell
cd src
make
make install
```
Or you can customize install directory by running command like "make install PREFIX=$TARGET_DIR".  
Noteworthy is that hiredis has been intergrated into cloredis, so you do not need to install hiredis separately.

## Document

Comming soon.

## Who Is Using Cloredis?

* [ofo 小黄车](http://www.ofo.so/#/) - ofo Inc., a Beijing-based bicycle sharing company

## Authors

* James Wei (weijianlhp@163.com)
