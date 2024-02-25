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
#include <thread>
#include <chrono>
#include <future>

#include <boost/asio.hpp>
#include <boost/beast.hpp>

#include "basic_log.hpp"

// std::ofstream to_log("./logs/log.txt", std::ios::app);

// pthread_mutex_t mlock = PTHREAD_MUTEX_INITIALIZER;

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;

using tcp = asio::ip::tcp;

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
  static void* sendPOST(Request req, int client_fd, int req_id);
  static void* sendCONNECT(Request req, int client_fd, int req_id);
  static void* sendGET(Request req, int client_fd, int req_id);
  static void* error502(int client_fd, int req_id);
  static void* error400(int client_fd, int req_id);
  static void* error404(int client_fd, int req_id);
};

void * Proxy::error404(int client_fd, int req_id){
  printError(req_id,"404 Not Found");
  std::string error_msg = "HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\nContent-Length: 0\r\n\r\n";
  int status = send(client_fd, error_msg.c_str(),error_msg.length(),0);
  if(status<0){
    putError("fail to send 404 error to client");
    return nullptr;
  }
  return nullptr;
}

void * Proxy::error400(int client_fd, int req_id){
  printError(req_id,"400 Bad Request");
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
  printError(req_id,"502 Bad Gateway");
  std::string error_msg = "HTTP/1.1 502 Bad Gateway\r\nContent-Type: text/html\r\nContent-Length: 0\r\n\r\n";
  int status = send(client_fd, error_msg.c_str(), error_msg.length(), 0);
  outRawMessage(std::to_string(req_id)+": Responding \""+error_msg);
  // if (status < 0)
  // {
  //   perror("send() failed");
  //   if (errno == EPIPE) {
  //       // Socket closed by remote end
  //       // Handle the situation appropriately
  //       putError("socket closed by the remote");
  //   } else if (errno == ENOTCONN) {
  //       putError("connection not established");
  //   } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
  //       putError("socket operation would block");
  //   } else if (errno == EINTR) {
  //       putError("operation interrupted by a signal");
  //   } else if (errno == ENOMEM) {
  //       putError("insufficient memory");
  //   } else if (errno == EFAULT) {
  //       putError("invalid send buffer");
  //   } else {
  //       putError("fail to send 502 error to client");
  //   }
  //   return nullptr;
  // }
  return nullptr;   
}

// void errorHandlingFunction(int client_fd) {
//     // 在这里执行错误处理逻辑，比如等待一段时间后重试、关闭连接等
//     std::this_thread::sleep_for(std::chrono::seconds(1));
//     std::cout << "Error handling function executed" << std::endl;
//     // 你可以在这里添加其他错误处理逻辑
// }

void * Proxy::sendCONNECT(Request req, int client_fd, int req_id){
  try{
  //outMessage("start to send CONNECT"+req.getPort()+req.getHost());
  outRawMessage(std::to_string(req_id)+": Requesting \""+req.getRequestLine()+"\" from "+req.getHost());

  Client client(req.getPort().c_str(), req.getHost().c_str());

  //not sure whether to send 200 OK or other response
  std::string res_msg = "HTTP/1.1 200 OK\r\n\r\n";
  int status = send(client_fd, res_msg.c_str(), res_msg.length(), 0);

  //outMessage("response sent to client"+std::to_string(req_id)+" "+req.getHost()+": "+res_msg);
  outRawMessage(std::to_string(req_id)+": Responding \""+res_msg);

  fd_set fd2;
  int fdMaxV = std::max(client.socket_fd, client_fd);

  while(true){
    FD_ZERO(&fd2);
    FD_SET(client.socket_fd, &fd2);
    FD_SET(client_fd, &fd2);

    int status = select(fdMaxV+1, &fd2, NULL, NULL, NULL);

    if(status<0){
      error502(client_fd, req_id);
      //outError("fail to select");
      close(client.socket_fd);
      return nullptr;
    }

    if(FD_ISSET(client_fd, &fd2)){
      //might need to change buffer size
      //and also need to initialize buffer
      char buffer[BUFSIZ];
      int bytes_received = recv(client_fd, buffer, BUFSIZ, 0);
      if(bytes_received<0){
        error502(client_fd, req_id);
        close(client.socket_fd);
        return nullptr;
      }
      if(bytes_received==0){
        return nullptr;
      }


      int status = send(client.socket_fd, buffer, bytes_received, 0);
      if(status<0){
        error502(client_fd, req_id);
        close(client.socket_fd);
        return nullptr;
      }
      if(bytes_received==0){
        return nullptr;
      }

    }

    if(FD_ISSET(client.socket_fd, &fd2)){
      char buffer[BUFSIZ];
      int bytes_received = recv(client.socket_fd, buffer, BUFSIZ, 0);
      if(bytes_received<0){
        error502(client_fd, req_id);
        close(client.socket_fd);
        return nullptr;
      }
      if(bytes_received==0){
        return nullptr;
      }

      int status = send(client_fd, buffer, bytes_received, 0);
      if(status<0){
        error502(client_fd, req_id);
        close(client.socket_fd);
        return nullptr;
      }
      if(bytes_received==0){
        return nullptr;
      }
    }
  }
  }catch(std::exception e){
    //outError("exception in sendCONNECT");
    printError(req_id,"exception in sendCONNECT");
    error404(client_fd, req_id);
    return nullptr;
  }

}

void * Proxy::sendPOST(Request req, int client_fd, int req_id){
  printNote(req_id,"start to send post");
  //outMessage("start to send post");

  Client client(req.getPort().c_str(), req.getHost().c_str());
  int status = send(client.socket_fd, req.getRequest().c_str(), req.getRequest().length(), 0);
  //outMessage("request sent to server"+std::to_string(req_id)+" "+req.getHost()+": "+req.getRequest());
  outRawMessage(std::to_string(req_id)+": Requesting \""+req.getRequestLine()+"\" from "+req.getHost());


  char res_buffer[1000];
  int bytes_received = recv(client.socket_fd, res_buffer, 1000, 0);
  if (bytes_received < 0)
  {
    error502(client_fd, req_id);
    printError(req_id,"fail to recieve response from outer server");
    //outError("fail to recieve response from outer server");
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
      if(res_str.find("\r\n0\r\n")!=std::string::npos){
        break;
      }
    }
  }

  Response final_res(res_str);
  //outMessage("response recieved from server"+std::to_string(req_id)+" "+req.getHost()+": "+final_res.getResponse());
  outRawMessage(std::to_string(req_id)+": Responding \""+final_res.getResponseLine()+"\"");


  int final_status = send(client_fd, res_str.c_str(), res_str.length(), 0);
  if (final_status < 0)
  {
    //outError("fail to send response from server to client");
    printError(req_id,"failed to send response to client");
    return nullptr;
  }
  return nullptr;
}

void * Proxy::sendGET(Request req, int client_fd, int req_id){
  try{
  //outMessage("start to send GET"+req.getPort()+req.getHost());
  printNote(req_id,"start to seng GET");
 // outRawMessage(std::to_string(req_id)+": Requesting \""+req.getRequestLine()+"\" from "+req.getHost());

  Client client(req.getPort().c_str(), req.getHost().c_str());

  if (cache_c.inCache(req))
  {
    Response *res = cache_c.getResponseFromCache(req, client.socket_fd, req_id);
    if (res == NULL)
    {
      //putError("fail to get response from cache");
      printError(req_id,"fail to get response from cache");
    }


    int status = send(client_fd, res->getResponse().c_str(), res->getResponse().length(), 0);
    if (status < 0)
    {
      //putError("fail to send response from cache to client");
      printError(req_id,"fail to send response from cache to client");
    }

    //outMessage(std::to_string(req_id)+"response sent from cache to client"+std::string(res->getStatus()));
    //close(client_fd);
    outRawMessage(std::to_string(req_id)+": esponse sent from cache to client");
    return nullptr;
  }

  outRawMessage(std::to_string(req_id)+": not in cache");

  int status = send(client.socket_fd, req.getRequest().c_str(), req.getRequest().length(), 0);

  //outMessage("request sent to server"+std::to_string(req_id)+" "+req.getHost()+": "+req.getRequest());
  //outRawMessage(std::to_string(req_id)+": Received \""+res->getResponse().c_str()+"\" from "+req.getHost());
  outRawMessage(std::to_string(req_id)+": Requesting \""+req.getRequestLine()+"\" from "+req.getHost());

  
  std::string res_get;
  beast::flat_buffer buffer;
  http::response<http::dynamic_body> res;


  try{
    asio::io_context ioc;
    tcp::socket socket(ioc);
    int new_client_fd = dup(client.socket_fd);
    socket.assign(tcp::v4(), new_client_fd);


    http::read(socket, buffer, res);

    // std::ostringstream ss;
    // ss<<res;
    // res_get = ss.str();
  }catch(std::exception &e){
    std::cerr<< "Exception: "<< e.what()<<std::endl;
    error502(client_fd, req_id);
    printError(req_id,"fail to recieve response from outer server");
    close(client.socket_fd);
    return nullptr;
  }

  std::ostringstream ss;
  ss<<res;
  res_get = ss.str();
  Response final_res(res_get);
  outRawMessage(std::to_string(req_id)+": Received \""+final_res.getResponseLine().c_str()+"\" from "+req.getHost());

  try{
    asio::io_context ioc;
    tcp::socket socket(ioc);
    int new_client_fd = dup(client_fd);
    socket.assign(tcp::v4(), new_client_fd);
    http::write(socket, res);
  }catch(const std::exception &e) {
    //std::cerr << "Exception: " << e.what() << std::endl;
    printError(req_id,"failed to send response to client");
    error502(client_fd, req_id);
    return nullptr;
  }


  // std::ostringstream ss;
  // ss<<res;
  // res_get = ss.str();
  // Response final_res(res_get);

  //outMessage(std::to_string(req_id)+"response sent from server to client"+std::string(final_res.getStatus()));
  outRawMessage(std::to_string(req_id)+": Responding \""+final_res.getResponseLine()+"\"");
  if(final_res.getChunked()==false){
    pthread_mutex_lock(&cache_lock);
    if(
    cache_c.cacheRec(final_res, req)){
      //to_log << "cache successfully" << std::endl;
      printNote(req_id,"cache successfully");
      //std::cout <<"cache successfully"<<std::endl;
    }
    else{
      // to_log << "cache failed" << std::endl;
      // std::cout <<"cache failed"<<std::endl;
      printNote(req_id,"cache failed");
    }
    pthread_mutex_unlock(&cache_lock);
  }
  //outMessage("after cache");
  //close(client_fd);
  return nullptr;

  }catch(std::exception e){
    //putError("exception in sendGET");
    printError(req_id,"exception in sendGET");
    //std::cerr<< "Exception: "<< e.what()<<std::endl;
    //error404(client_fd, req_id);
    // if(e.what()!=NULL){
    //   outError(e.what());
    // }
    return nullptr;
  }
}

void* Proxy::recvRequest(void *args)
{
  thread_data *data = static_cast<thread_data*>(args);
  int client_fd = data->client_fd;
  int req_id = data->req_id;
  std::string client_ip = data->client_ip;

  std::string req_get;

  asio::io_context ioc;
  tcp::socket socket(ioc);
  boost::system::error_code ec;
  int new_client_fd = dup(client_fd);
  socket.assign(tcp::v4(), new_client_fd, ec);
  if(ec){
    printError(req_id,"failed to assign socket");
    //std::cerr<<"Failed to assign socket"<<std::endl;
    return nullptr;
  }
  try{
    beast::flat_buffer buffer;
    http::request<http::string_body> req;
    http::read(socket, buffer, req, ec);
    if(ec){
      printError(req_id,"failed to read request:"+ec.message());
      //std::cerr<<"Failed to read request: "<<ec.message()<<std::endl;
      //to_log<<"Failed to read request"<<ec.message()<<std::endl;
      return nullptr;
    }

    std::ostringstream ss;
    ss<<req;
    req_get = ss.str();
  }catch(std::exception &e){
    printError(req_id,"Exception: "+std::string(e.what()));
    //std::cerr<< "Exception: "<< e.what()<<std::endl;
    return nullptr;
  }

  Request reqPtr(req_get, req_id);

  std::time_t ctime = std::time(nullptr);
  std::tm* ctime_utc = std::gmtime(&ctime);
  std::string ctime_str = std::asctime(ctime_utc);

  outRawMessage(std::to_string(req_id)+": \""+reqPtr.getRequestLine()+"\" from "+reqPtr.getHost()+" @ "+ctime_str);

  if(reqPtr.getMethod()=="GET"){
    sendGET(reqPtr, client_fd, req_id);
  }
  if(reqPtr.getMethod()=="POST"){
    sendPOST(reqPtr, client_fd, req_id);
  }
  if(reqPtr.getMethod()=="CONNECT"){
    sendCONNECT(reqPtr, client_fd, req_id);
  }

  if(reqPtr.getMethod()!="GET" && reqPtr.getMethod()!="POST" && reqPtr.getMethod()!="CONNECT"){
    error400(client_fd, req_id);
  }

  close(client_fd);
  //outMessage("test done"+std::to_string(req_id));
  outRawMessage(std::to_string(req_id)+": Tunnel closed");
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
    //outMessage("waiting for connection out");
    client_fd = serverP.acceptConnection(client_ip);
    //outMessage("connection accepted");
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