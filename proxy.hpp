#ifndef POXY_HPP
#define POXY_HPP

#include "errorhandle.hpp"
#include "server.hpp"
#include "response.hpp"
#include "request.hpp"
#include "cache.hpp"
#include "tothread.hpp"

#include <sys/stat.h>
#include <fstream>
#include <fcntl.h>
#include <vector>
#include "basic_log.hpp"

// std::ofstream to_log("./logs/log.txt", std::ios::app);

// pthread_mutex_t mlock = PTHREAD_MUTEX_INITIALIZER;

class Proxy
{
  const char *host_name;
  const char *port;
  int sc_fd;
  int cc_fd;
  Cache cache;
  Server serverP;

  public:
  Proxy(const char *host, const char *port):host_name(host), port(port),cache(100),serverP(port){}
  Proxy():host_name(NULL), port(NULL),cache(100),serverP(port){}
  Proxy(const char *port):host_name(NULL), port(port), cache(100), serverP(port){
    //serverP.initServer();
  }

  void Deamonlize();
  void mainProcess();

  //functions for threads
  static void* recvRequest(void *args);
  static void* sendGET(Request req, int client_fd, int req_id);
};

void * Proxy::sendGET(Request req, int client_fd, int req_id){
  Client client(req.getPort().c_str(), req.getHost().c_str());

  

}

void* Proxy::recvRequest(void *args)
{
  thread_data *data = static_cast<thread_data*>(args);
  int client_fd = data->client_fd;
  int req_id = data->req_id;
  std::string client_ip = data->client_ip;

  //recieve request from client

  std::vector<char> request_msg(1024*1024);
  //request_msg.resize(1000*1000);
  int bytes_recieved = recv(client_fd, request_msg.data(), request_msg.size(), 0);
  if(bytes_recieved < 0){
    putError("fail to recieve request from client");
    //close(client_fd);
    //return NULL;
  }
  if(bytes_recieved == 0){
    //putError("client closed connection");
    close(client_fd);
    return NULL;
  }

  std::string req_str(request_msg.begin(), request_msg.begin() + bytes_recieved);
  Request req(req_str, req_id);

  if(req.getMethod()=="GET"){
    
  }
  if(req.getMethod()=="POST"){

  }
  if(req.getMethod()=="CONNECT"){

  }
  close(client_fd);
  outMessage("test done");
  pthread_exit(NULL);
  return NULL;
  
}


void Proxy::Deamonlize()
{
  pid_t m_pid = fork();
  if (m_pid < 0)
  {
    outError("fail to fork()");
    exit(EXIT_FAILURE);
  }
  if (m_pid > 0)
  {
    std::cout << "Daemon process created with pid: " << m_pid << std::endl;
    exit(EXIT_SUCCESS);
  }
  pid_t n_sid = setsid();
  if (n_sid < 0)
  {
    outError("fail to create new session");
    exit(EXIT_FAILURE);
  }

  if (chdir("/") < 0)
  {
    outError("fail to change directory to /");
    exit(EXIT_FAILURE);
  }

  int null_fd = open("/dev/null", O_RDWR);
  if (null_fd != -1)
  {
    dup2(null_fd, STDIN_FILENO);
    dup2(null_fd, STDOUT_FILENO);
    dup2(null_fd, STDERR_FILENO);
    if (null_fd > 2)
    {
      close(null_fd);
    }
  }
  else
  {
    outError("fail to open /dev/null");
    exit(EXIT_FAILURE);
  }

  umask(0);

  pid_t l_pid = fork();
  if (l_pid < 0)
  {
    outError("fail to fork()");
    exit(EXIT_FAILURE);
  }
  if (l_pid > 0)
  {
    // std::cout << "Daemon process created with pid: " << pid << std::endl;
    exit(EXIT_SUCCESS);
  }

  mainProcess();
}

void Proxy::mainProcess()
{
  serverP.initServer();
  int req_id = -1;
  int client_fd;
  std::string client_ip;
  outMessage("Proxy started");
  while(true){
    outMessage("waiting for connection out");
    client_fd = serverP.acceptConnection(client_ip);
    outMessage("connection accepted");
    if(client_fd < 0){
      outError("fail to accept connection");
      continue;
    }

    pthread_mutex_lock(&mlock);
    req_id++;
    pthread_mutex_unlock(&mlock);

    thread_data to_thread_data;

    to_thread_data.req_id = req_id;
    to_thread_data.client_fd = client_fd;
    to_thread_data.client_ip = client_ip;

    pthread_t thread;
    pthread_create(&thread, NULL, recvRequest, &to_thread_data);

  }
  
}
#endif