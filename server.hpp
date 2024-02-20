#ifndef _SERVER_HPP_
#define _SERVER_HPP_

#include<netdb.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<cstring>
#include<cstdio>
#include<cstdlib>

#include "errorhandle.hpp"

class Server{
  public:
    int socket_fd;
    int status;
    struct addrinfo host_info;
    struct addrinfo *host_info_list;
    const char *port;
    const char *host_name;

    Server(const char *port, const char *host_name):socket_fd(0),status(0), host_info_list(NULL) ,port(port),host_name(host_name){initServer();};

    void initServer();
    int acceptConnection(std::string &client_ip);
    int portNum(int socket_fd){
      struct sockaddr_in sin;
      socklen_t len = sizeof(sin);
      if(getsockname(socket_fd, (struct sockaddr *)&sin, &len) == -1){
        putError("getsockname error in portNum");
      }
      return ntohs(sin.sin_port);
    }

    ~Server(){
      freeaddrinfo(host_info_list);
      close(socket_fd);
    };
    //int client_fd;
};

class Client{
  public:
    int socket_fd;
    int status;
    struct addrinfo host_info;
    struct addrinfo *host_info_list;
    const char *port;
    const char *host_name;

    Client(const char *port, const char *host_name):socket_fd(0),status(0), host_info_list(NULL) ,port(port),host_name(host_name){initClient();};

    void initClient();
    //void connectToServer();

    ~Client(){
      freeaddrinfo(host_info_list);
      close(socket_fd);
    };
    //int client_fd;
};


void Server::initServer(){
  memset(&host_info, 0, sizeof(host_info));
  host_info.ai_family = AF_UNSPEC;
  host_info.ai_socktype = SOCK_STREAM;
  host_info.ai_flags = AI_PASSIVE;
  status = getaddrinfo(host_name, port, &host_info, &host_info_list);
  if(status != 0){
    putError("getaddrinfo error in initServer");
  }
  socket_fd = socket(host_info_list->ai_family, host_info_list->ai_socktype, host_info_list->ai_protocol);
  if(socket_fd == -1){
    putError("socket error in initServer");
  }
  int yes = 1;
  status = setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
  status = bind(socket_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
  if(status == -1){
    putError("bind error in initServer");
  }
  status = listen(socket_fd, 100);
  if(status == -1){
    putError("listen error in initServer");
  }
}

int Server::acceptConnection(std::string &client_ip){
  struct sockaddr_storage socket_addr;
  socklen_t addr_size = sizeof(socket_addr);
  int client_fd = accept(socket_fd, (struct sockaddr *)&socket_addr, &addr_size);
  if(client_fd == -1){
    putError("accept error in acceptConnection");
  }

  //char ipstr[INET6_ADDRSTRLEN];
  struct sockaddr_in *s = (struct sockaddr_in *)&socket_addr;
  client_ip = inet_ntoa(s->sin_addr);

  return client_fd;
}

void Client::initClient(){
  memset(&host_info, 0, sizeof(host_info));
  host_info.ai_family = AF_UNSPEC;
  host_info.ai_socktype = SOCK_STREAM;
  status = getaddrinfo(host_name, port, &host_info, &host_info_list);
  if(status != 0){
    putError("getaddrinfo error in initClient");
  }
  socket_fd = socket(host_info_list->ai_family, host_info_list->ai_socktype, host_info_list->ai_protocol);
  if(socket_fd == -1){
    putError("socket error in initClient");
  }
  status = connect(socket_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
  if(status == -1){
    putError("connect error in initClient");
  }
}

#endif