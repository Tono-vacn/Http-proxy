CC = g++
CFLAGS = -g -pthread
TARGET = proxy
SRCS = main.cpp cache.hpp proxy.hpp server.hpp request.hpp response.hpp tothread.hpp errorhandle.hpp basic_log.hpp
OBJS = $(SRCS:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

%.o: %.cpp
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(TARGET) main.o