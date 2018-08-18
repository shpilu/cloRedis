# cloredis
Cloredis is a simple, flexsible, easy-to-use C++ client library for the Redis database, based on hiredis.

Usage like ====>
RedisManager* manager = RedisManager::instance();
manager->Connect("127.0.0.1", 6379, 1000, "oforedis"); 
RedisConnection conn = manager->Get(1);
std::string value = conn->Do("GET %s", "my_key").toString();
bool exist = conn->Do("EXISTS %s", key).toBool();                                                                                       
conn->Do("SET %s %d", "my_key", 8);
...
// Yes! You don't need to release this connection as cloredis will do it automatically
