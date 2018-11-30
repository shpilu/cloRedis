[English Version](README.md)

cloRedis<div id="top"></div>
====

Cloredis是一个简单易用的Redis C++客户端库，致力于提供一套比其它各类redis C++库更方便的redis存取方式。

它天然支持内存池与连接池，以提高存取效率，并支持redis连接在不需要时自动放回连接池(<strong>cloredis特有的功能</strong>)，同时完全接管了内存分配与释放操作，避免了内存泄漏与redis连接泄漏的风险，最后在设计上特意考量了生产环境常用的一主多从Redis部署场景，从而可以更方便地选择访问主库或者从库。

这些特性使得cloredis特别适用于生产环境，目前cloRedis在ofo小黄车得到了应用，效果不错。

在设计上，cloRedis的底层实现是[hiredis](https://github.com/redis/hiredis.git)，并参考了部分业界领先的开源库，比如连接池的设计参考了[redigo](https://github.com/gomodule/redigo.git)(Golang最流行的redis库)，redis连接自动回收功能参考了[brpc](https://github.com/brpc/brpc.git)(百度开源的高性能RPC框架)，主从redis库访问参考了[redis3m](https://github.com/luca3m/redis3m.git)，同时有它自己的特性。

* [主要特性](#features)
* [Cloredis使用实例](#usage)
* [安装](#installation)
* [API参考](#reference)
* [谁在使用cloRedis](#users)
* [作者](#authors)

## 主要特性<div id="features"></div>

* 天然支持连接池与内存池 -- cloRedis的每个redis连接都有一个对应的连接池，所有操作都可抽象为“从连接池取连接--执行redis命令”的过程；当连接回收时其开辟的内存块会进入内存池，而分配新连接时也优先从内存池分配 
* redis连接自动回收 -- <strong>当redis连接不再使用时会自动回收，不需要手动将连接放回连接池</strong>，使开发者从连接管理与内存管理中完全解放出来
* 支持一主多从 -- 一般redis库只封装了一个redis库的连接，cloredis兼顾了一主多从的部署场景，能选择访问主库或者特定的从库，使得访问一主多从的场景更加简单 
* 针对每个每个db建连接池 -- 不用执行select命令选择数据库(性能和操作上的一点优化)，且只有某个db用到时才会建对应的连接池 

## Cloredis使用实例<div id="usage"></div>

基本用法:
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
    // 注意！redis连接conn1会自动回收(在其析构函数中)，不用手动释放
    RedisConnection conn1 = manager->Get(1); 
    conn1->Do("SET tkey1 %d", 100);
    std::cout << conn1->Do("GET tkey1").toString() << std::endl; // 100
}
...

```
高级用法:
```C++
// Specify connection pool options and access master/slave instance
#include <cloredis/cloredis.h>
using namespace cloris;

// 自定义连接池属性
ConnectionPoolOption option;
option.max_idle = 40;
option.max_active = 100;
option.idle_timeout_ms = 180 * 1000;   
RedisManager *manager = RedisManager::instance();
// '172.17.224.212:6379' 是主库, "172.17.224.212:6380,172.17.224.212:6381" 是从库 
if (!manager->InitEx("172.17.224.212:6379", "172.17.224.212:6380,172.17.224.212:6381", "cloris520", 100, &option)) {
    std::cout << "init redis manager failed" << std::endl;
    return;
}

{
    // access redis master instance, you can also write as 'RedisConnection conn1 = manager->Get(5, NULL, MASTER)'
    // 默认是取主库
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
    // 指定取从库
    conn2 = manager->Get(5, NULL, SLAVE);
    std::cout << conn2->Do("GET tkey").toString() << std::endl;
}
```

## 安装<div id="installation"></div>
Cloredis目前只在linux环境经过验证，在linux系统下cloredis的安装命令为
``` shell
# Cloredis默认安装在/usr/local/cloredis目录
cd src
make
sudo make install

```
在make install这一步，你可以通过指定PREFIX参数自定义安装路径(make install PREFIX=$TARGET_DIR)

安装cloredis以后，可以参考如下方式来编译引入cloredis的程序(假设cloredis安装在'/usr/local/cloredis'目录)
```C++
g++ tutorial.cc -I/usr/local/cloredis/include -L/usr/local/cloredis/lib/ -lcloredis -o main  -std=c++11 -Wl,-rpath=/usr/local/cloredis/lib
```
cloredis对其他库没有依赖，你不需要安装其他依赖库。

## API参考<div id="reference"></div> 

Cloredis的API文档后续将会提供，你可以先参考[教程](https://github.com/shpilu/cloRedis/blob/master/src/example/tutorial.cc)或者直接看源码了解cloredis的用法。

## 谁在使用cloRedis<div id="users"></div>

* [ofo 小黄车](http://www.ofo.so/#/)

## 作者<div id="authors"></div>

* James Wei (weijianlhp@163.com)   

[返回顶部](#top)
