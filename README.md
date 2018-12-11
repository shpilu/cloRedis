
cloRedis<div id="top"></div>
=====
尽管github上有众多的redis客户端库，我们还是设计了cloRedis，在于cloRedis有这些超过一般redis客户端库的特性:
* **天然支持连接池且redis连接使用后自动回收**(cloRedis独有的特性)
* **天然支持一主多从**(大部分redis客户端库没有的特性)

[中文版](README_cn.md)

## Introduction

Cloredis is a simple, high-performance C++ Client for Redis with native support for connection pool and master/slave instances access. 

Cloredis's main goal is to <strong>provide a more convenient way to access redis in production environment than general C++ client</strong>.

Cloredis is not a standalone project, the design of which references, integrates and takes advantage of some leading Redis Clients in C++/Golang language, e.g. [hiredis](https://github.com/redis/hiredis.git), [redigo](https://github.com/gomodule/redigo.git), [redis3m](https://github.com/luca3m/redis3m.git) and [brpc](https://github.com/brpc/brpc.git), and has its own features.

* [Features](#features)
* [Usage at a glance](#usage)
* [Installation](#installation)
* [API Reference](#reference)
* [Who Is Using CloRedis?](#users)
* [Authors](#authors)
## Features<div id="features"></div>

* Support connection pool and memory pool naturally -- All connection operations are based on connection pool.
* Automatic memory management and connection reclaim -- You do not need any memory operation, nor need you put the connection back to connection pool when it is no longer required.  
* Support master/slave replication -- You can choose access master or slave instance in redis replication environment.

Cloredis's features make it well adapted for production environment. Now cloredis is used in ofo Inc. and works very well.

## Usage at a glance<div id="usage"></div>

Basic usage:
``` C++
//
// The following shows how to get a connection from connection pool, select specific db 
// and run redis command by using redis connection
//
#include <cloredis/cloredis.h>
using namespace cloris;

RedisManager *manager = RedisManager::instance();
if (!manager->Init("172.17.224.212:6379", "cloris520", 100)) {
    std::cout << "init redis manager failed" << std::endl;
    return;
}
{
    // Note! You do not need to put 'conn1' back to connection pool as cloredis will 
    // do it automatically in RedisConnection's destructor function
    RedisConnection conn1 = manager->Get(1); 
    conn1->Do("SET tkey1 %d", 100);
    std::cout << conn1->Do("GET tkey1").toString() << std::endl; // 100
}
...
```
Advanced usage:
```C++
// Specify connection pool options and access master/slave instance
#include <cloredis/cloredis.h>
using namespace cloris;

ConnectionPoolOption option;
option.max_idle = 40;
option.max_active = 100;
option.idle_timeout_ms = 180 * 1000;   
RedisManager *manager = RedisManager::instance();
// '172.17.224.212:6379' is master host, and "172.17.224.212:6380,172.17.224.212:6381" is two slave hosts
if (!manager->InitEx("172.17.224.212:6379", "172.17.224.212:6380,172.17.224.212:6381", "cloris520", 100, &option)) {
    std::cout << "init redis manager failed" << std::endl;
    return;
}

{
    // access redis master instance, you can also write as 'RedisConnection conn1 = manager->Get(5, NULL, MASTER)'
    RedisConnection conn1 = manager->Get(5);
    conn1->Do("NOCOMMAND city %s", "Beijing");
    // use 'ok' or 'error' method to determine whether the command has run success
    // true
    if (conn1->error()) {
        std::cout << "run command error:" << conn1->err_str() << std::endl;
    } else {
        std::cout << "run command ok" << std::endl;
    }
    conn1->Do("set tkey 100");
    sleep(5);
    // access redis slave instance
    conn2 = manager->Get(5, NULL, SLAVE);
    std::cout << conn2->Do("GET tkey").toString() << std::endl;
}

```

## Installation<div id="installation"></div>
On Linux system you can build cloredis compile and runtime environment by running the following commands:
``` shell
# by default, cloredis will install in /usr/local/cloredis
cd src
make
sudo make install

```
Or you can customize install directory by running command "make install PREFIX=$TARGET_DIR".  

The following example shows how to compile with cloredis(assume cloredis is installed in '/usr/local/cloredis' directory)
```C++
g++ tutorial.cc -I/usr/local/cloredis/include -L/usr/local/cloredis/lib/ -lcloredis -o main  -std=c++11 -Wl,-rpath=/usr/local/cloredis/lib
```

Noteworthy is that hiredis has been intergrated into cloredis, so you do not need to install hiredis separately.

## API Reference<div id="reference"></div> 

Cloredis's API document will come soon, and now you can refer to [tutorial](https://github.com/shpilu/cloRedis/blob/master/src/example/tutorial.cc) temporarily.

## Who Is Using CloRedis?<div id="users"></div>

* [ofo 小黄车](http://www.ofo.so/#/) - ofo Inc., a Beijing-based bicycle sharing company.

## Authors<div id="authors"></div>

* James Wei (weijianlhp@163.com)   
* Virgood (suchoy0607@gmail.com)   
Please contact us if you have trouble using cloRedis.

[Go back to top](#top)
