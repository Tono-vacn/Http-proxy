#ifndef POXY_HPP
#define POXY_HPP

#include "errorhandle.hpp"
#include "server.hpp"
#include "response.hpp"
#include "request.hpp"

class Proxy
{
  const char *host_name;
  const char *port;
  int sc_fd;
  int cc_fd;

  public:
  Proxy(const char *host, const char *port):host_name(host), port(port){}
  Proxy():host_name(NULL), port(NULL){}
  Proxy(const char *port):host_name(NULL), port(port){}

  void Deamonlize();


#endif