#include"request.hpp"

int main(){
  std::string raw_request = "CONNECT vscode-sync.trafficmanager.net:443 HTTP/1.1\r\nHost: vscode-sync.trafficmanager.net:443\r\nProxy-Connection: keep-alive\r\nUser-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Code/1.86.2 Chrome/118.0.5993.159 Electron/27.2.3 Safari/537.36";
  Request req(raw_request, 1);
  std::cout << req.getRequest() << std::endl;
  std::cout << req.getHost() << std::endl;
  std::cout << req.getPort() << std::endl;
  std::cout << req.getURI() << std::endl;
  std::cout << req.getMethod() << std::endl;
  std::cout << req.getID() << std::endl;
  std::cout << req.getMaxStale() << std::endl;
  std::cout << req.getCache() << std::endl;


    std::string str = "Hello, world!";
    auto pos = str.find("world", std::string::npos);
    if (pos == std::string::npos) {
        std::cout << "npos" << std::endl;
    } else {
        std::cout << "Found at position: " << pos << std::endl;
    }

  return 0;
}