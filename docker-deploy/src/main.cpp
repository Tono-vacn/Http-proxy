#include "proxy.hpp"
#include "server.hpp"

int main(int argc, char * argv[]) {
  std::string port = "12345";
  Proxy myProxy(port.c_str());
  myProxy.Deamonlize();
  // myProxy.mainProcess();

  return 0;
}