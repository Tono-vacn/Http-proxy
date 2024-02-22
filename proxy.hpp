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
  Proxy(const char *port):host_name(NULL), port(port), cache(100), serverP(port){}

  void Deamonlize();
  void mainProcess();
  static void* handleRequest(void *args);
};

void* Proxy::handleRequest(void *args)
{
  // thread_data *data = (thread_data *)request;
  // if (request.getMethod() == "GET")
  // {
  //   if (cache.inCache(request))
  //   {
  //     // Respond from cache
  //   }
  //   else
  //   {
  //     // Forward request to server
  //     // client?
  //     // Response response = client.sendRequest(request);
  //     // Cache the response
  //     // cache.cacheRec(response,request);
  //   }
  // }
}
void Proxy::Deamonlize()
{
  pid_t m_pid = fork();
  if (m_pid < 0)
  {
    // pthread_mutex_lock(&mlock);
    // to_log << "Error: fail to fork()" << strerror(errno) << std::endl;
    // pthread_mutex_unlock(&mlock);
    // std::cerr << "Error: fail to fork()" << strerror(errno) << std::endl;
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
  int req_id = -1;
  int client_fd;
  std::string client_ip;
  while(true){
    client_fd = serverP.acceptConnection(client_ip);
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
    pthread_create(&thread, NULL, handleRequest, &to_thread_data);

  }
  
}
#endif