#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include "errorhandle.hpp"
#include "server.hpp"

class Response
{
  std::string response;
  std::string response_status;
  std::string response_code;
  std::string expires; 
  std::string etag;
  std::string date;
  std::string last_modified;
  std::string content_type;
  int content_length = 0;

  bool no_cache = false;
  bool no_store = false;
  bool private_ = false;
  bool public_ = false;
  bool must_revalidate = false;
  bool is_chunked = false;
  bool have_max_stale = false;
  int max_stale = 0;
  bool have_max_age = false;
  int max_age = 0;

  // std::ofstream &to_log;
  // pthread_mutex_t &mlock;

public:

  Response(std::string raw_response):response(raw_response){
    readStatus();
    readEtag();
    readDate();
    readExpires();
    readLastModified();
    // readContentType();
    readContentLength();
    readCacheControl();
    checkChunked();
    //checkCache();
    // checkRevalidate();
    // checkMaxStale();
  }
  void readStatus();
  void readEtag();
  void readDate();
  void readExpires();
  void readLastModified();
  // void readContentType();
  void readContentLength();
  // void checkCache();
  // void checkRevalidate();
  // void checkMaxStale();
  void readCacheControl();
  
  void checkChunked();

  bool haveMaxAge()
  {
    return have_max_age;
  }

  bool haveMaxStale()
  {
    return have_max_stale;
  }

  bool getPublic()
  {
    return public_;
  }

  bool getPrivate()
  {
    return private_;
  }

  std::string getDate()
  {
    return date;
  }

  std::string getLastModified()
  {
    return last_modified;
  }

  std::string getExpires()
  {
    return expires;
  }

  std::string getContentType()
  {
    return content_type;
  }

  int getContentLength()
  {
    return content_length;
  }

  std::string getEtag()
  {
    return etag;
  }

  std::string getStatus()
  {
    return response_status;
  }

  std::string getCode()
  {
    return response_code;
  }

  int getMaxStale()
  {
    return max_stale;
  }

  int getMaxAge()
  {
    return max_age;
  }

  bool getNoCache()
  {
    return no_cache;
  }

  bool getNoStore()
  {
    return no_store;
  }

  bool getRevalidate()
  {
    return must_revalidate;
  }

  bool getChunked()
  {
    return is_chunked;
  }

  std::string getResponse()
  {
    return response;
  }
};

void Response::readStatus()
{
  size_t pos = response.find(" ");
  size_t pos_end = response.find(" ", pos + 1);
  if (pos == std::string::npos || pos_end == std::string::npos)
  {
    putError("Invalid response, fail to read status in response");
  }
  response_code = response.substr(pos + 1, pos_end - pos - 1);
  size_t end = response.find("\r\n");
  if (end == std::string::npos)
  {
    putError("Invalid response, fail to read status in response");
  }
  response_status = response.substr(0, end);
}

void Response::readDate()
{
  size_t pos = response.find("Date: ");
  if(pos == std::string::npos){
    return;
  }
  size_t pos_end = response.find("\r\n", pos);
  if (pos_end == std::string::npos)
  {
    putError("Invalid response, fail to read date in response");
  }
  date = response.substr(pos + 6, pos_end - pos - 6);
}
  
void Response::readExpires()
{
  size_t pos = response.find("Expires: ");
  if(pos == std::string::npos){
    return;
  }
  size_t pos_end = response.find("\r\n", pos);
  if (pos_end == std::string::npos)
  {
    putError("Invalid response, fail to read expires in response");
  }
  expires = response.substr(pos + 9, pos_end - pos - 9);
}

void Response::readEtag()
{
  size_t pos = response.find("ETag: ");
  if(pos == std::string::npos){
    return;
  }
  size_t pos_end = response.find("\r\n", pos);
  if (pos_end == std::string::npos)
  {
    putError("Invalid response, fail to read etag in response");
  }
  etag = response.substr(pos + 6, pos_end - pos - 6);
}

void Response::readLastModified()
{
  size_t pos = response.find("Last-Modified: ");
  if(pos == std::string::npos){
    return;
  }
  size_t pos_end = response.find("\r\n", pos);
  if (pos_end == std::string::npos)
  {
    putError("Invalid response, fail to read last-modified in response");
  }
  last_modified = response.substr(pos + 15, pos_end - pos - 15);
}

void Response::readContentLength()
{
  size_t pos = response.find("Content-Length: ");
  if(pos == std::string::npos){
    return;
  }
  size_t pos_end = response.find("\r\n", pos);
  if (pos_end == std::string::npos)
  {
    putError("Invalid response, fail to read content-length in response");
  }
  content_length = stoul(response.substr(pos + 16, pos_end - pos - 16));
}

void Response::checkChunked()
{
  size_t pos = response.find("chunked");
  if(pos != std::string::npos){
    is_chunked = true;
  }
}

void Response::readCacheControl(){
  size_t pos = response.find("Cache-Control: ");
  if(pos == std::string::npos){
    return;
  }
  size_t pos_end = response.find("\r\n", pos);
  if (pos_end == std::string::npos)
  {
    putError("Invalid response, fail to read cache-control in response");
  }
  std::string cache_control = response.substr(pos, pos_end - pos);

  size_t max_stale_pos = cache_control.find("max-stale=");
  if(max_stale_pos != std::string::npos){
    have_max_stale = true;
    size_t max_stale_end = cache_control.find(",", max_stale_pos);
    if(max_stale_end == std::string::npos){
      max_stale_end = pos_end;
    }
    max_stale = stoul(cache_control.substr(max_stale_pos + 10, max_stale_end - max_stale_pos - 10));
  }

  size_t max_age_pos = cache_control.find("max-age=");
  if(max_age_pos != std::string::npos){
    have_max_age = true;
    size_t max_age_end = cache_control.find(",", max_age_pos);
    if(max_age_end == std::string::npos){
      max_age_end = pos_end;
    }
    max_age = stoul(cache_control.substr(max_age_pos + 8, max_age_end - max_age_pos - 8));
  }

  if(cache_control.find("no-cache") != std::string::npos){
    no_cache = false;
  }

  if(cache_control.find("must-revalidate") != std::string::npos){
    must_revalidate = true;
  }

  if(cache_control.find("no-store") != std::string::npos){
    no_store = true;
  }

  if(cache_control.find("public") != std::string::npos){
    public_ = true;
  }

  if(cache_control.find("private") != std::string::npos){
    private_ = true;
  }




}


#endif
