#include "response.hpp"
#include <iostream>

int main(){
  std::string raw_response = "HTTP/1.1 200 OK\r\nDate: Mon, 19 Feb 2024 12:00:00 GMT\r\nExpires: Tue, 20 Feb 2024 12:00:00 GMT\r\nContent-Type: text/html; charset=UTF-8\r\nContent-Length: 6789\r\nCache-Control: max-age=3600, max-stale=600, must-revalidate, no-cache, private\r\nETag: \"33a64df551425fcc55e4d42a148795d9f25f89d4\"\r\nLast-Modified: Mon, 12 Feb 2024 12:30:00 GMT\r\n\r\n<html>";
  Response res(raw_response);
  std::cout << res.getDate() << std::endl;
  std::cout << res.getEtag() << std::endl;
  // std::cout << res.getExpires() << std::endl;
  std::cout << res.getLastModified() << std::endl;
  std::cout << res.getContentLength() << std::endl;
  std::cout << res.getNoCache()<< std::endl;
  std::cout << res.getChunked() << std::endl;
  // std::cout << res.getNoCache() << std::endl;
  // std::cout << res.getNoStore() << std::endl;
  std::cout << res.getPublic() << std::endl;
  std::cout << res.getRevalidate() << std::endl;
  std::cout << res.getMaxStale() << std::endl;
  std::cout << res.getMaxAge() << std::endl;
  std::cout << res.getResponse() << std::endl;
  // outError("test");
  // std::cout << res.getResponseCode() << std::endl;
  // std::cout << res.getResponseStatus() << std::endl;
  return 0;
}