#ifndef REQUEST_HPP
#define REQUEST_HPP

#include<string>
#include<iostream>
#include<exception>
#include<cstring>

#include "errorhandle.hpp"

class Request{
  std::string request;
  std::string method;
  std::string uri;
  std::string host_name;
  std::string port;
  bool is_cache = false;
  int max_stale = NULL;
  int id = NULL;

  public:
    Request(std::string raw_request, int id):request(raw_request),id(id){
      // parseRequest();
      readMethod();
      readURI();
      readHost();
      checkCache();
    }

    bool getCache(){
      return is_cache;
    }

    std::string getRequest(){
      return request;
    }
    std::string getHost(){
      return host_name;
    } 
    std::string getPort(){
      return port;
    }
    std::string getURI(){
      return uri;
    }
    std::string getMethod(){
      return method;
    }
    int getID(){
      return id;
    }

    int getMaxStale(){
      return max_stale;
    }



    void readMethod();
    //void readPost();
    void readURI();
    void readHost();
    //void readId();
    void checkCache();

};

void Request::readMethod(){
  size_t pos = request.find(" ");
  if(pos == std::string::npos){
    putError("Invalid request, fail to read method");
  }
  method = request.substr(0, pos);
  //request = request.substr(pos+1);
}

void Request::readHost(){
  //std::string request = "Host: vscode-sync.trafficmanager.net:443\r\n";
  size_t pos = request.find("Host: ");
  size_t pos_end = request.find("\r\n", pos);
  if(pos == std::string::npos||pos_end == std::string::npos){
    putError("Invalid request, fail to read host");
  }
  // request = request.substr(pos+6);
  // pos = request.find("\r\n");
  std::string temp_host_name = request.substr(pos+6, pos_end-pos-6);
  size_t pos_colon = temp_host_name.find(":");
  if(pos_colon == std::string::npos){
    host_name = temp_host_name;
    port = "80";
  }else{
    host_name = temp_host_name.substr(0, pos_colon);
    port = temp_host_name.substr(pos_colon+1, pos_end-pos_colon-1);
  }
  //request = request.substr(pos+2);
}

void Request::readURI(){
  size_t pos = request.find(" ");
  if(pos == std::string::npos){
    putError("Invalid request, fail to read uri 1");
  }
  //request = request.substr(pos+1);
  size_t pos_end = request.find(" ", pos+1);
  if(pos == std::string::npos){
    putError("Invalid request, fail to read uri 2");
  }
  uri = request.substr(pos+1, pos_end-pos-1);
  // uri = request.substr(0, pos);
  // request = request.substr(pos+1);
}

void Request::checkCache(){
  size_t cache_pos = request.find("Cache-Control: ");
  if(cache_pos == std::string::npos){
    is_cache = false;
    return;
  }else{
    size_t cache_end = request.find("\r\n", cache_pos);
    if(cache_end == std::string::npos){
      putError("Invalid request, fail to read cache");
    }
    size_t max_stale_pos = request.find("max-stale=", cache_pos);
    if(max_stale_pos == std::string::npos){
      is_cache = false;
      return;
      }
    else{
      is_cache = true;
      size_t max_stale_end = request.find(",", max_stale_pos);
      if(max_stale_end != std::string::npos){
        max_stale = std::stoi(request.substr(max_stale_pos+10, max_stale_end-max_stale_pos-10));
      }else{
        max_stale = std::stoi(request.substr(max_stale_pos+10, cache_end-max_stale_pos-10));
      }
    }
  }
}

#endif