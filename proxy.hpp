#ifndef POXY_HPP
#define POXY_HPP

#include "errorhandle.hpp"
#include "server.hpp"
#include "response.hpp"
#include "request.hpp"

#include <sys/stat.h>
#include<fstream>
#include<fcntl.h>
#include<vector>

std::ofstream to_log("./logs/log.txt", std::ios::app);

pthread_mutex_t mlock = PTHREAD_MUTEX_INITIALIZER;

void outError(const char *msg)
{
  pthread_mutex_lock(&mlock);
  to_log << "Error: " << msg << strerror(errno) << std::endl;
  pthread_mutex_unlock(&mlock);
  std::cerr << "Error: " << msg << strerror(errno) << std::endl;
}

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
  void mainProcess(); 
};

void Proxy::Deamonlize()
{
  pid_t m_pid = fork();
  if(m_pid < 0)
  {
    // pthread_mutex_lock(&mlock);
    // to_log << "Error: fail to fork()" << strerror(errno) << std::endl;
    // pthread_mutex_unlock(&mlock);
    // std::cerr << "Error: fail to fork()" << strerror(errno) << std::endl;
    outError("fail to fork()");
    exit(EXIT_FAILURE);
  }
  if(m_pid > 0)
  {
    std::cout << "Daemon process created with pid: " << m_pid << std::endl;
    exit(EXIT_SUCCESS);
  }
  pid_t n_sid = setsid();
  if(n_sid < 0)
  {
    outError("fail to create new session");
    exit(EXIT_FAILURE);
  }

  if(chdir("/") < 0)
  {
    outError("fail to change directory to /");
    exit(EXIT_FAILURE);
  }

  int null_fd = open("/dev/null", O_RDWR);
  if(null_fd !=-1){
    dup2(null_fd, STDIN_FILENO);
    dup2(null_fd, STDOUT_FILENO);
    dup2(null_fd, STDERR_FILENO);
    if(null_fd > 2)
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
  if(l_pid < 0)
  {
    outError("fail to fork()");
    exit(EXIT_FAILURE);
  }
  if(l_pid > 0)
  {
    //std::cout << "Daemon process created with pid: " << pid << std::endl;
    exit(EXIT_SUCCESS);
  }

  mainProcess();
}

void Proxy::mainProcess()
{
  Server serverP(port);
}



#endif