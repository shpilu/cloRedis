cloredis
=====

## Introduction

Cloredis is a simple, flexsible, easy-to-use C++ client library for the Redis database, based on hiredis. As designed, accessing redis should be as easy as possible, like "Get a connection, run command", or even "Run command". That's to say, you don't need to connect redis or release the connection by yourself, and cloredis is where it should be. 

## Features

* Automatic memory management.
* Support connection pool naturally.
* Easier way to run Lua script.
* High rubustness and fault-tolerance.
* Support C++11 and upper only.

## Usage

entry-level usage:
``` C++
RedisManager* manager = RedisManager::instance();
manager->Connect("127.0.0.1", 6379, 1000, "password"); 
RedisConnection conn = manager->Get(2);
std::string my_value = conn->Do("GET %s", "my_key").toString();
int exist = conn->Do("EXISTS %s", "my_key").toInt32();
conn->Do("SET %s %d", "my_key", 8);
// You do not need to release 'conn'
```

## Install

On Linux system you can build the library using the following commands:
``` shell
cd src
make
make install
```
## Document

Comming soon.

## Companies using cloredis

* [ofo 小黄车](http://www.ofo.so/#/) - ofo Inc., a Beijing-based bicycle sharing company

## Authors

* James Wei (weijianlhp@163.com)
