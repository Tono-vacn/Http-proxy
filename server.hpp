#ifndef _SERVER_HPP_
#define _SERVER_HPP_

#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <cstdio>
#include <cstdlib>

#include "errorhandle.hpp"
#include "request.hpp"
#include "response.hpp"

class Server
{
public:
  int socket_fd;
  int status;
  struct addrinfo host_info;
  struct addrinfo *host_info_list;
  const char *port;
  const char *host_name;
  // std::ofstream & to_log;
  // pthread_mutex_t & mlock;

  Server(const char *port, const char *host_name) : socket_fd(0), status(0), host_info_list(NULL), port(port), host_name(host_name) { initServer(); };
  Server(const char *port) : socket_fd(0), status(0), host_info_list(NULL), port(port), host_name(NULL) { }//initServer(); };

  void initServer(){
  outMessage("Initializing server");
  memset(&host_info, 0, sizeof(host_info));
  host_info.ai_family = AF_UNSPEC;
  host_info.ai_socktype = SOCK_STREAM;
  host_info.ai_flags = AI_PASSIVE;
  status = getaddrinfo(host_name, port, &host_info, &host_info_list);
  if (status != 0)
  {
    putError("getaddrinfo error in initServer");
  }
  socket_fd = socket(host_info_list->ai_family, host_info_list->ai_socktype, host_info_list->ai_protocol);
  if (socket_fd == -1)
  {
    putError("socket error in initServer");
  }

  // bind?

  outMessage("Binding socket");

  int yes = 1;
  status = setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
  if (status == -1)
  {
    putError("setsockopt error in initServer");
  }
  

  status = bind(socket_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
  if (status == -1)
  {
    putError("bind error in initServer");
  }
  status = listen(socket_fd, 100);
  if (status == -1)
  {
    putError("listen error in initServer");
  }
  outMessage(("Server is listening on port " +hostName(socket_fd) +std::to_string(portNum(socket_fd))).c_str());
}

  int acceptConnection(std::string &client_ip)
  {
  struct sockaddr_storage socket_addr;
  socklen_t addr_size = sizeof(socket_addr);

  outMessage("Waiting for connection in");

  int client_fd = accept(socket_fd, (struct sockaddr *)&socket_addr, &addr_size);
  if (client_fd == -1)
  {
    putError("accept error in acceptConnection");
  }

  outMessage("Connection accepted");

  // char ipstr[INET6_ADDRSTRLEN];
  struct sockaddr_in *s = (struct sockaddr_in *)&socket_addr;
  client_ip = inet_ntoa(s->sin_addr);

  return client_fd;
};


  int portNum(int socket_fd)
  {
    struct sockaddr_in sin;
    socklen_t len = sizeof(sin);
    if (getsockname(socket_fd, (struct sockaddr *)&sin, &len) == -1)
    {
      putError("getsockname error in portNum");
    }
    return ntohs(sin.sin_port);
  }

  std::string hostName(int socket_fd)
  {
    struct sockaddr_in sin;
    socklen_t len = sizeof(sin);
    if (getsockname(socket_fd, (struct sockaddr *)&sin, &len) == -1)
    {
      putError("getsockname error in hostName");
    }
    return inet_ntoa(sin.sin_addr);
  }

  ~Server()
  {
    freeaddrinfo(host_info_list);
    close(socket_fd);
  };
  // int client_fd;
};

class Client
{
public:
  int socket_fd;
  int status;
  struct addrinfo host_info;
  struct addrinfo *host_info_list;
  const char *port;
  const char *host_name;
  // std::ofstream & to_log;
  // pthread_mutex_t & mlock;

  Client(const char *port, const char *host_name) : socket_fd(0), status(0), host_info_list(NULL), port(port), host_name(host_name) { initClient(); };
  Client() : socket_fd(0), status(0), host_info_list(NULL), port("8080"), host_name("localhost"){ initClient(); }

  void initClient();
  // void connectToServer();
  //Response sendRequest(Request& request);

  ~Client()
  {
    freeaddrinfo(host_info_list);
    close(socket_fd);
  };
  // int client_fd;
};

// Response Client::sendRequest(Request& request){
//   int status = send(socket_fd, request.getRequest().c_str(), request.getRequest().length(), 0);
//   if(status<0){
//     putError("Failed to send request from client");
//     //return Response();
//   }
//   char buffer[4096];
//   int bytes_received = recv(socket_fd, buffer, sizeof(buffer), 0);
//   if(bytes_received<0){
//     putError("Failed to receive response from server");
//     //return Response();
//   }
//   Response response(std::string(buffer, bytes_received));
//   return response;
// }

//void Server::initServer()


//int Server::acceptConnection(std::string &client_ip)


void Client::initClient()
{
  memset(&host_info, 0, sizeof(host_info));
  host_info.ai_family = AF_UNSPEC;
  host_info.ai_socktype = SOCK_STREAM;
  status = getaddrinfo(host_name, port, &host_info, &host_info_list);
  if (status != 0)
  {
    putError("getaddrinfo error in initClient");
  }
  socket_fd = socket(host_info_list->ai_family, host_info_list->ai_socktype, host_info_list->ai_protocol);
  if (socket_fd == -1)
  {
    putError("socket error in initClient");
  }

  // bind?

  status = connect(socket_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
  if (status == -1)
  {
    putError("connect error in initClient");
  }
}

#endif