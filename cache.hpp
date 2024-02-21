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
  bool checkResponse(Response res);
  
};

bool Cache::checkResponse(Response res){
  if(res.getNoStore() == true){
    outError("No store in response");
    return false;
  }

  if(res.getPublic() == false && res.getPrivate() == true){
    outError("response is private");
    return false;
  }

  if(res.getEtag().empty() && res.getLastModified().empty()){
    outError("No etag or last modified in response");
    return false;
  }

  //add any other checks here

  return true;

}

bool Cache::cacheRec(Response res, Request req){
  if(checkResponse(res) == false){
    return false;
  }

  if(cachePool.size() == max_size){
    std::string k_r = cacheQueue.front();
    cacheQueue.pop();
    cachePool.erase(k_r);
  }
    
}
  

#endif