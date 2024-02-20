#ifndef ERRORHANDLE_HPP
#define ERRORHANDLE_HPP

#include<iostream>

void putError(const char* error){
  std::cerr << error << std::endl;
  throw  std::exception();
  //exit(EXIT_FAILURE);
}

#endif