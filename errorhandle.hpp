#ifndef ERRORHANDLE_HPP
#define ERRORHANDLE_HPP

// #include<iostream>
#include<fstream>
#include "basic_log.hpp"
#include <iostream>

//std::ofstream to_log("./logs/log.txt", std::ios::app);

void putError(const char* error){//, std::ofstream& to_log, pthread_mutex_t &mlock){
  pthread_mutex_lock(&mlock);
  to_log << error << std::endl;
  pthread_mutex_unlock(&mlock);
  std::cerr << error << std::endl;
  throw std::exception();
  //exit(EXIT_FAILURE);
}

void outError(const char *msg)
{
  pthread_mutex_lock(&mlock);
  to_log << "Error: " << msg <<std::endl;// strerror(errno) << std::endl;
  pthread_mutex_unlock(&mlock);
  std::cerr << "Error: " << msg <<std::endl;// strerror(errno) << std::endl;
}

void outMessage(const char *msg)
{
  pthread_mutex_lock(&mlock);
  to_log << "Message: " << msg << std::endl;
  pthread_mutex_unlock(&mlock);
  std::cout << "Message: " << msg << std::endl;
}

#endif