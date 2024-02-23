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

Cache cache_c(100);

class Proxy
{
  const char *host_name;
  const char *port;
  int sc_fd;
  int cc_fd;
  // Cache cache;
  Server serverP;

  public:
  Proxy(const char *host, const char *port):host_name(host), port(port),serverP(port){}
  Proxy():host_name(NULL), port(NULL),serverP(port){}
  Proxy(const char *port):host_name(NULL), port(port),serverP(port){
    //serverP.initServer();
  }

  void Deamonlize();
  void mainProcess();

  //functions for threads
  static void* recvRequest(void *args);
  static void* sendGET(Request req, int client_fd, int req_id);
  static void* error502(int client_fd, int req_id);
  static void* error400(int client_fd, int req_id);
};

void * Proxy::error400(int client_fd, int req_id){
  std::string error_msg = "HTTP/1.1 400 Bad Request\r\nContent-Type: text/html\r\nContent-Length: 0\r\n\r\n";
  int status = send(client_fd, error_msg.c_str(), error_msg.length(), 0);
  if (status < 0)
  {
    putError("fail to send 400 error to client");
    return nullptr;
  }
  return nullptr;
}

void * Proxy::error502(int client_fd, int req_id){
  std::string error_msg = "HTTP/1.1 502 Bad Gateway\r\nContent-Type: text/html\r\nContent-Length: 0\r\n\r\n";
  int status = send(client_fd, error_msg.c_str(), error_msg.length(), 0);
  if (status < 0)
  {
    putError("fail to send 502 error to client");
    return nullptr;
  }
  return nullptr;   
}

void * Proxy::sendGET(Request req, int client_fd, int req_id){
  
  Client client(req.getPort().c_str(), req.getHost().c_str());

  if (cache_c.inCache(req))
  {
    Response *res = cache_c.getResponseFromCache(req, client.socket_fd);
    if (res == NULL)
    {
      putError("fail to get response from cache");
    }

    int status = send(client_fd, res->getResponse().c_str(), res->getResponse().length(), 0);
    if (status < 0)
    {
      putError("fail to send response from cache to client");
    }

    outMessage(std::to_string(req_id)+"response sent from cache to client"+std::string(res->getStatus()));
    close(client_fd);
    return nullptr;
  }

  int status = send(client.socket_fd, req.getRequest().c_str(), req.getRequest().length(), 0);

  outMessage("request sent to server"+std::to_string(req_id)+" "+req.getHost()+": "+req.getRequest());
  
  char res_buffer[1000];
  int bytes_received = recv(client.socket_fd, res_buffer, 1000, 0);
  if (bytes_received < 0)
  {
    error502(client_fd, req_id);
    outError("fail to recieve response from outer server");
    close(client.socket_fd);
    return nullptr;
  }

  std::string res_str(res_buffer, res_buffer + bytes_received);
  Response res_head(res_str);
  char res_buffer2[BUFSIZ];
  if(res_head.getContentLength()==0){
    while(true){
      bytes_received = recv(client.socket_fd, res_buffer2, BUFSIZ, 0);
      if(bytes_received<0){
        error502(client_fd, req_id);
        close(client.socket_fd);
        return nullptr;
      }
      if(bytes_received==0){
        break;
      }
      res_str.append(res_buffer2, bytes_received);
      if(res_str.find("\r\n0\r\n")!=std::string::npos){
        break;
      }
    }
  }else{
    int lth = res_head.getContentLength()+res_head.getHeaderLength();
    while(bytes_received<lth){
      int bytes = recv(client.socket_fd, res_buffer2, BUFSIZ, 0);
      if(bytes<0){
        error502(client_fd, req_id);
        close(client.socket_fd);
        return nullptr;
      }
      if(bytes==0){
        break;
      }
      bytes_received += bytes;
      res_str.append(res_buffer2, bytes);
    }
  }

  Response final_res(res_str);
  outMessage("response recieved from server"+std::to_string(req_id)+" "+req.getHost()+": "+final_res.getResponse());

  int final_status = send(client_fd, res_str.c_str(), res_str.length(), 0);
  if (final_status < 0)
  {
    outError("fail to send response from server to client");
  }

  outMessage(std::to_string(req_id)+"response sent from server to client"+std::string(final_res.getStatus()));
  
  cache_c.cacheRec(final_res, req);
  
  close(client_fd);
}

void* Proxy::recvRequest(void *args)
{
  thread_data *data = static_cast<thread_data*>(args);
  int client_fd = data->client_fd;
  int req_id = data->req_id;
  std::string client_ip = data->client_ip;

  //recieve request from client


  // is recv loop needed here?
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