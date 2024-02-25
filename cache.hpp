#ifndef CACHE_HPP
#define CACHE_HPP

#include"request.hpp"
#include"response.hpp"
#include"errorhandle.hpp"
#include"basic_log.hpp"

#include<signal.h>
#include<sys/stat.h>

#include<fstream>
#include<fcntl.h>
#include<unistd.h>
#include<ctime>
#include<sstream>
#include<iomanip>

#include<unordered_map>
#include<map>
#include<queue>

class Cache{
  size_t max_size;
  //std::ofstream & to_log;
  std::queue<std::string> cacheQueue;
  std::map<std::string, Response> cachePool;
public:
  Cache(size_t max_size):max_size(max_size){};
  static std::time_t formatTime(const std::string & time){
    std::tm tm = {};
    std::istringstream s(time);
    s >> std::get_time(&tm, "%a, %d %b %Y %H:%M:%S GMT");
    if(s.fail()){
      outError("Failed to parse time");
      //return -1;
    }
    tm.tm_isdst = 0;
    return timegm(&tm);
  };
  bool cacheRec(Response res, Request req);
  bool checkResponse(Response res);
  bool inCache(Request req);
  bool checkResponseInCache(Request req, Response res, int res_id);
  Response *getResponseFromCache(Request req, int fd, int req_id);
  //bool  sendResponseFromCache(Request * req, int fd);
  std::string generateValidateRequest(Request req, Response * res);
    
  
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

bool Cache::inCache(Request req){
  std::string k = req.getURI();
  if(cachePool.find(k) == cachePool.end()){
    return false;
  }
  return true;
}

bool Cache::cacheRec(Response res, Request req){
  std::cout <<"cache start"<<std::endl;
  if(checkResponse(res) == false){
    return false;
  }
std::cout <<"after check response"<<std::endl;
  if(cachePool.size() == max_size){
    std::string k_r = cacheQueue.front();
    cachePool.erase(k_r);
    cacheQueue.pop();
  }
std::cout <<"before get URI"<<std::endl;
  std::string k_a = req.getURI();
  cacheQueue.push(k_a);
  cachePool[k_a] = res;
std::cout <<"before cahce return"<<std::endl;
  return true;

    
}

bool Cache::checkResponseInCache(Request req, Response res, int req_id){
//check time first
// might need more adjustments

std::time_t now_time = time(nullptr);
std::tm now_utc;
gmtime_r(&now_time, &now_utc);

std::time_t expire_tm;
std::tm expire_tm_utc;

std::time_t response_tm = formatTime(res.getDate());

if(res.getNoCache()){
  to_log <<req_id<< " : No cache in response" << std::endl;
  return false;
}

if(res.haveMaxAge()){
  expire_tm = response_tm+res.getMaxAge();
  gmtime_r(&expire_tm, &expire_tm_utc);
}else if(!res.getExpires().empty()){
  expire_tm = formatTime(res.getExpires());
  gmtime_r(&expire_tm, &expire_tm_utc);
}else{
  //to_log <<req_id<< " : No max age or expires in response, valid" << std::endl;
  return true;
}

if(!res.getRevalidate()&&res.haveMaxStale()){
  expire_tm += res.getMaxStale();
  gmtime_r(&expire_tm, &expire_tm_utc);
}

if(now_time > expire_tm){
  //to_log <<req_id<< " : Expired at" << std::asctime((std::tm *)&expire_tm_utc)<<std::endl;
  outRawMessage(std::to_string(req_id)+": in cache, but expired at "+std::asctime((std::tm *)&expire_tm_utc));
  return false;
}

//to_log <<req_id<< " :in cache, valid" << std::asctime((std::tm *)&expire_tm_utc)<<std::endl;
return true;

}

std::string Cache::generateValidateRequest(Request req, Response * res){
  std::string validate_req = "GET " + req.getURI() + " HTTP/1.1\r\n";
  validate_req += "Host: " + req.getHost() + "\r\n";
  if(!(res->getEtag().empty())){
    validate_req += "If-None-Match: " + res->getEtag() + "\r\n";
  }else{
    validate_req += "If-Modified-Since: " + res->getLastModified() + "\r\n";
  }
  validate_req += "Connection: close\r\n\r\n";
  return validate_req;
}

Response * Cache::getResponseFromCache(Request req, int fd, int req_id){
  std::string k = req.getURI();
  Response * res = &cachePool[k];
  if(checkResponseInCache(req, *res, req.getID())){
    outRawMessage(std::to_string(req_id)+": in cache, valid");
    return res;
  }

  std::string validate_req = generateValidateRequest(req, res);
  outMessage("Sending validate request in Cache::getResponseFromCache");
  outMessage(validate_req.c_str());
  int status = send(fd, validate_req.c_str(), validate_req.length(), 0);
  if(status<0){
    putError("Failed to send validate request");
  }


  // do i need to implement a recv loop here?
  std::vector<char> char_buffer(1024);
  std::string response_recv;
  while((status = recv(fd, char_buffer.data(), char_buffer.size(), 0)) > 0){
    response_recv.append(char_buffer.begin(), char_buffer.begin()+status);
  }

  Response * new_res = new Response(response_recv);
  outMessage("Received response from Server in Cache::getResponseFromCache");
  outMessage(new_res->getEtag().c_str());

  if(new_res->getCode() == "304"){
    to_log << req.getID() << " : Not modified" << std::endl;
    return res;
  }
  if(new_res->getCode() == "200"){
    to_log << req.getID() << " : Modified" << std::endl;
    cacheRec(*new_res, req);
    return new_res;
  }

  putError("Invalid response code in Cache::getResponseFromCache");
  //this one should not be reached
  exit(EXIT_FAILURE);

  // if(cachePool.find(k) == cachePool.end()){
  //   return nullptr;
  // }
  // return &cachePool[k];
}
  

#endif