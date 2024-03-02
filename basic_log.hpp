#ifndef BASIC_LOG_HPP
#define BASIC_LOG_HPP

#include<fstream>
#include<fcntl.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<unistd.h>
#include<cstring>

//#include "cache.hpp"

//this one should be /var/log/erss/proxy.log, but can be modified later
std::ofstream to_log("./logs/log.txt");//, std::ios::app);

// std::ofstream to_log("/var/log/erss/proxy.log");

pthread_mutex_t mlock = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t cache_lock = PTHREAD_MUTEX_INITIALIZER;
//Cache cache_c(100);

#endif