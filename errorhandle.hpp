#ifndef ERRORHANDLE_HPP
#define ERRORHANDLE_HPP

// #include<iostream>
#include<fstream>

//std::ofstream to_log("./logs/log.txt", std::ios::app);

void putError(const char* error){//, std::ofstream& to_log, pthread_mutex_t &mlock){
  pthread_mutex_lock(&mlock);
  to_log << error << std::endl;
  pthread_mutex_unlock(&mlock);
  std::cerr << error << std::endl;
  throw  std::exception();
  //exit(EXIT_FAILURE);
}

#endif