#ifndef BASIC_LOG_HPP
#define BASIC_LOG_HPP

#include<fstream>
#include<fcntl.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<unistd.h>
#include<cstring>

std::ofstream to_log("./logs/log.txt", std::ios::app);

pthread_mutex_t mlock = PTHREAD_MUTEX_INITIALIZER;

#endif