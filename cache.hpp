#ifndef CACHE_HPP
#define CACHE_HPP

#include"request.hpp"
#include"response.hpp"

#include<signal.h>
#include<sys/stat.h>

#include<fstream>
#include<fcntl.h>

#include<unordered_map>
#include<map>
#include<queue>

class Cache{
  size_t max_size;
  std::ofstream & to_log;
  std::queue<std::string> cacheQueue;
  std::map<std::string, Response> cachePool;
public:
  Cache(size_t max_size, std::ofstream & to_log):max_size(max_size), to_log(to_log){};
  bool cacheRec(Response res, Request req);
  
};

#endif