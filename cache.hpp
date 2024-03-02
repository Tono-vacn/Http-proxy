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

#include <boost/asio.hpp>
#include <boost/beast.hpp>

#include<unordered_map>
#include<map>
#include<queue>

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;

using tcp = asio::ip::tcp;

class Cache{
  size_t max_size;
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
  bool cacheRec(Response res, Request req, int req_id);
  bool checkResponse(Response res, int req_id);
  bool inCache(Request req, int req_id);
  bool checkResponseInCache(Request req, Response res, int req_id);
  Response *getResponseFromCache(Request req, int fd, int req_id);
  //bool  sendResponseFromCache(Request * req, int fd);
  std::string generateValidateRequest(Request req, Response * res, int req_id);
    
  
};

bool Cache::checkResponse(Response res, int req_id){
  if(res.getNoStore() == true){
    //outMessage("No store in response");
    // printNote(req_id, "No store in response");
    outRawMessage(std::to_string(req_id)+": not cacheable because no-store");
    return false;
  }

  if(res.getPublic() == false && res.getPrivate() == true){
    // outMessage("response is private");
    outRawMessage(std::to_string(req_id)+": not cacheable because private");
    return false;
  }

  if(res.getEtag().empty() && res.getLastModified().empty()){
    // outMessage("No etag or last modified in response");
    outRawMessage(std::to_string(req_id)+": not cacheable because no etag or last-modified");
    return false;
  }

  //add any other checks here

  return true;

}

bool Cache::inCache(Request req, int req_id){
  std::string k = req.getURI();
  if(cachePool.find(k) == cachePool.end()){
    return false;
  }
  return true;
}

bool Cache::cacheRec(Response res, Request req, int req_id){
  //std::cout <<"cache start"<<std::endl;
  printNote(req_id, "cache start");
  if(checkResponse(res, req_id) == false){
    return false;
  }
//std::cout <<"after check response"<<std::endl;
  printNote(req_id, "after check response");
  if(cachePool.size() == max_size){
    std::string k_r = cacheQueue.front();
    cachePool.erase(k_r);
    cacheQueue.pop();
  }
// std::cout <<"before get URI"<<std::endl;
  printNote(req_id, "before get URI");
  std::string k_a = req.getURI();
  cacheQueue.push(k_a);
  cachePool[k_a] = res;

  if(!res.getExpires().empty()){
    // std::cout <<"no expires"<<std::endl;
    outRawMessage(std::to_string(req_id)+": cached, expires at "+res.getExpires());
    printNote(req_id, "before cahce return");
    return true;
  }

  if(res.getRevalidate()){
    // std::cout <<"no revalidate"<<std::endl;
    outRawMessage(std::to_string(req_id)+": cached, but requires re-validation");
    printNote(req_id, "before cahce return");
    return true;
  }

// std::cout <<"before cahce return"<<std::endl;

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
  outRawMessage(std::to_string(req_id)+": in cache, requires validation");
  //to_log <<req_id<< " : No cache in response" << std::endl;
  return false;
}

if(res.haveMaxAge()){
  expire_tm = response_tm+res.getMaxAge();
  gmtime_r(&expire_tm, &expire_tm_utc);
}else if(!res.getExpires().empty()){
  expire_tm = formatTime(res.getExpires());
  gmtime_r(&expire_tm, &expire_tm_utc);
}else{
  if(res.getRevalidate()){
  //to_log <<req_id<< " : No max age or expires in response, valid" << std::endl;
  outRawMessage(std::to_string(req_id)+": in cache, requires validation");
  return false;
  }
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

std::string Cache::generateValidateRequest(Request req, Response * res, int req_id){
  std::string validate_req = "GET " + req.getURI() + " HTTP/1.1\r\n";
  validate_req += "Host: " + req.getHost() + "\r\n";
  if(!(res->getEtag().empty())){
    validate_req += "If-None-Match: " + res->getEtag() + "\r\n";
  }
  if(!(res->getLastModified().empty())){
    validate_req += "If-Modified-Since: " + res->getLastModified() + "\r\n";
  }
  validate_req += "Connection: close\r\n\r\n";
  return validate_req;
}

Response * Cache::getResponseFromCache(Request req, int fd, int req_id){
  std::string k = req.getURI();
  Response * res = &cachePool[k];
  //if(checkResponseInCache(req, *res, req.getID())){
  if(checkResponseInCache(req, *res, req_id)){
    outRawMessage(std::to_string(req_id)+": in cache, valid");
    return res;
  }

  std::string validate_req = generateValidateRequest(req, res, req_id);
  //outMessage("Sending validate request in Cache::getResponseFromCache");
  //outMessage(validate_req.c_str());
  printNote(req_id, "Sending validate request in Cache::getResponseFromCache");
  outRawMessage(std::to_string(req_id)+": Requesting"+validate_req.substr(0, validate_req.find("\r\n"))+" from "+req.getHost());
  int status = send(fd, validate_req.c_str(), validate_req.length(), 0);
  if(status<0){
    //putError("Failed to send validate request");
    outRawMessage(std::to_string(req_id)+": Failed to send validate request");
    //printError(req_id, "Failed to send validate request");
    return nullptr;
  }

  // refactor this with boost

  std::string req_get;

  asio::io_context ioc;
  tcp::socket socket(ioc);
  boost::system::error_code ec;
  int new_client_fd = dup(fd);
  socket.assign(tcp::v4(), new_client_fd, ec);
  if(ec){
    // std::cerr<<"Failed to assign socket"<<std::endl;
    printError(req_id, "Failed to assign socket in Cache::getResponseFromCache");
    printError(req_id, ec.message());
    return nullptr;
  }
  try{
    beast::flat_buffer buffer;
    http::response<http::dynamic_body> req;
    http::read(socket, buffer, req, ec);
    if(ec){
      // std::cerr<<"Failed to read request: "<<ec.message()<<std::endl;
      // to_log<<"Failed to read request"<<ec.message()<<std::endl;
      printError(req_id, "Failed to read response in Cache::getResponseFromCache");
      printError(req_id, ec.message());
      return nullptr;
    }

    std::ostringstream ss;
    ss<<req;
    req_get = ss.str();
  }catch(std::exception &e){
    // std::cerr<< "Exception: "<< e.what()<<std::endl;
    printError(req_id, e.what());
    return nullptr;
  }

  Response * new_res;
  ////
  try{
    new_res = new Response(req_get);
  }catch(std::exception &e){
    printError(req_id, e.what());
    return nullptr;
  }
  printNote(req_id, "Received response from Server in Cache::getResponseFromCache");
  outRawMessage(std::to_string(req_id)+": Recieved"+new_res->getResponseLine()+" from "+req.getHost());
  printNote(req_id, "Etag: "+new_res->getEtag());
  //outMessage("Received response from Server in Cache::getResponseFromCache");
  //outMessage(new_res->getEtag().c_str());

  if(new_res->getCode() == "304"){
    to_log << req.getID() << " : Not modified" << std::endl;
    return res;
  }
  if(new_res->getCode() == "200"){
    to_log << req.getID() << " : Modified" << std::endl;
    cacheRec(*new_res, req, req_id);
    return new_res;
  }

  //putError("Invalid response code in Cache::getResponseFromCache");
  printError(req_id, "Invalid response code in Cache::getResponseFromCache");

  //this one should not be reached
  exit(EXIT_FAILURE);

}
  

#endif