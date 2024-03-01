#ifndef TOTHREAD_HPP
#define TOTHREAD_HPP

#include <string>

typedef struct thread_data_t
{
  int req_id;
  int client_fd;
  std::string client_ip;
} thread_data;

#endif