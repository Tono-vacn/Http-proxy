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

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;
constexpr size_t bufferSize = 4096; 

using tcp = asio::ip::tcp;

Cache cache_c(100);

// int req_id = -1;

class Session : public std::enable_shared_from_this<Session> {
    beast::tcp_stream server_stream_;
    asio::posix::stream_descriptor client_stream_;
    std::array<char, bufferSize> buffer_;
    asio::io_context &ioc_;

public:
    Session(asio::io_context& ioc, tcp::socket server_socket, int client_fd)
    : server_stream_(std::move(server_socket)),
      client_stream_(ioc, client_fd) ,
      ioc_(ioc)
      {}

    void start(int req_id) {
        readFromClient(req_id);
        readFromServer(req_id);
    }

private:
    void readFromClient(int req_id) {
        auto self(shared_from_this());
        client_stream_.async_read_some(asio::buffer(buffer_),
            [this, self, req_id](boost::system::error_code ec, std::size_t length) {
                // if(flag == 1){
                //   return;
                // }
                if (!ec) {
                  // print
                    writeToServer(length, req_id);
                }else if(ec == asio::error::eof||ec == boost::system::errc::connection_reset){
                    // close(client_fd);
                    //outMessage("test done"+std::to_string(req_id));
                    // printNote(req_id,"thread done");
                    // pthread_exit(NULL);
                    //flag = 1;
                    ioc_.stop();
                    return;
                }
                else {
                  printError(req_id,"Read from client failed: "+ec.message());
                    //std::cerr << "Read from client failed: " << ec.message() << std::endl;
                }
                return;
            });

    }

    void writeToServer(std::size_t length, int req_id) {
        auto self(shared_from_this());
        asio::async_write(server_stream_, asio::buffer(buffer_, length),
            [this, self, req_id](boost::system::error_code ec, std::size_t length) {

                if (!ec) {
                    readFromClient(req_id);
                }else if(ec == asio::error::eof||ec == boost::system::errc::connection_reset){
                    ioc_.stop();
                    return;
                }else {
                  printError(req_id,"Write to server failed: "+ec.message());
                    //std::cerr << "Write to server failed: " << ec.message() << std::endl;
                }
                return;
            });
    }

    void readFromServer(int req_id) {
        auto self(shared_from_this());
        server_stream_.async_read_some(asio::buffer(buffer_),
            [this, self, req_id](boost::system::error_code ec, std::size_t length) {
                if (!ec) {
                    writeToClient(length,req_id);
                }else if(ec == asio::error::eof||ec == boost::system::errc::connection_reset){
                    //flag = 1;
                    ioc_.stop();
                    return;
                } else {
                  printError(req_id,"Read from server failed: "+ec.message());
                //std::cerr << "Read from server failed: " << ec.message() << std::endl;
            }
            return;
            });
    }

    void writeToClient(std::size_t length, int req_id) {
        auto self(shared_from_this());
        asio::async_write(client_stream_, asio::buffer(buffer_, length),
            [this, self, req_id](boost::system::error_code ec, std::size_t /*length*/) {
                if (!ec) {
                    readFromServer(req_id);
                }else if(ec == asio::error::eof||ec == boost::system::errc::connection_reset){
                    ioc_.stop();
                    return;
                } else {
                  printError(req_id,"Write to client failed: "+ec.message());
                    //std::cerr << "Write to client failed: " << ec.message() << std::endl;
                }
              return;
            });
    }

};


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
std::string getFirstLine(std::string& str){
  size_t pos1 = str.find("\r\n");
  size_t pos2 = str.find("\n");
  size_t pos = pos1<pos2?pos1:pos2;
  if(pos==std::string::npos){
    return str;
  }
  else{
    return str.substr(0, pos);
  }
}
void * Proxy::error404(int client_fd, int req_id){
  printError(req_id,"404 Not Found");
  std::string error_msg = "HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\nContent-Length: 0\r\n\r\n";
  int status = send(client_fd, error_msg.c_str(),error_msg.length(),0);
  if(status<0){
    printError(req_id,"fail to send 404 error to client");
    return nullptr;
  }

  outRawMessage(std::to_string(req_id)+": Responding \""+"HTTP/1.1 404 Not Found\"");
  return nullptr;
}

void * Proxy::error400(int client_fd, int req_id){
  printError(req_id,"400 Bad Request");
  std::string error_msg = "HTTP/1.1 400 Bad Request\r\nContent-Type: text/html\r\nContent-Length: 0\r\n\r\n";
  int status = send(client_fd, error_msg.c_str(), error_msg.length(), 0);

  if (status < 0)
  {
    printError(req_id, "fail to send 400 error to client");
    return nullptr;
  }

  outRawMessage(std::to_string(req_id)+": Responding \""+"HTTP/1.1 400 Bad Request\"");
  return nullptr;
}

void * Proxy::error502(int client_fd, int req_id){
  printError(req_id,"502 Bad Gateway");
  std::string error_msg = "HTTP/1.1 502 Bad Gateway\r\nContent-Type: text/html\r\nContent-Length: 0\r\n\r\n";
  int status = send(client_fd, error_msg.c_str(), error_msg.length(), 0);

  if(status<0){
    printError(req_id,"fail to send 502 error to client");
    return nullptr;
  }
  outRawMessage(std::to_string(req_id)+": Responding \""+"HTTP/1.1 502 Bad Gateway\"");
  return nullptr;   
}

void * Proxy::sendCONNECT( Request req, int client_fd, int req_id) {
    asio::io_context ioc;
    //std::unique_ptr<asio::io_context::work> work = std::make_unique<asio::io_context::work>(ioc);
    tcp::resolver resolver(ioc);
    auto endpoints = resolver.resolve(req.getHost(), req.getPort());
    tcp::socket server_socket(ioc);
    asio::connect(server_socket, endpoints);

    outRawMessage(std::to_string(req_id)+": Requesting \""+req.getRequestLine()+"\" from "+req.getHost());

    std::string res_msg = "HTTP/1.1 200 OK\r\n\r\n";
    int status = send(client_fd, res_msg.c_str(), res_msg.length(), 0);
    if (status < 0)
    {
      printError(req_id,"fail to send 200 OK to client");
      return nullptr;
    }
    outRawMessage(std::to_string(req_id)+": Responding \""+getFirstLine(res_msg)+"\"");//res_msg.substr(0, res_msg.find("\r\n"))+"\"");


    auto session = std::make_shared<Session>(ioc, std::move(server_socket), client_fd);
    session->start(req_id);

    ioc.run();
    return nullptr;
}

void * Proxy::sendPOST(Request req, int client_fd, int req_id){
  printNote(req_id,"start to send post");
  //outMessage("start to send post");

  Client client(req.getPort().c_str(), req.getHost().c_str());
  int status = send(client.socket_fd, req.getRequest().c_str(), req.getRequest().length(), 0);
  //outMessage("request sent to server"+std::to_string(req_id)+" "+req.getHost()+": "+req.getRequest());
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

  //Response final_res(res_str);
  try{
  Response final_res(res_get);
  //outMessage("response recieved from server"+std::to_string(req_id)+" "+req.getHost()+": "+final_res.getResponse());
  outRawMessage(std::to_string(req_id)+": Received \""+final_res.getResponseLine()+"\" from "+req.getHost());
  outRawMessage(std::to_string(req_id)+": Responding \""+final_res.getResponseLine()+"\"");


  //int final_status = send(client_fd, res_str.c_str(), res_str.length(), 0);
  int final_status = send(client_fd, res_get.c_str(), res_get.length(), 0);
  if (final_status < 0)
  {
    //outError("fail to send response from server to client");
    printError(req_id,"failed to send response to client");
    return nullptr;
  }
  return nullptr;
  }catch(std::exception &e){
    //putError("exception in sendPOST");
    printError(req_id,"exception in sendPOST");
    return nullptr;
  }
}

void * Proxy::sendGET(Request req, int client_fd, int req_id){
  try{
  //outMessage("start to send GET"+req.getPort()+req.getHost());
  printNote(req_id,"start to seng GET");
 // outRawMessage(std::to_string(req_id)+": Requesting \""+req.getRequestLine()+"\" from "+req.getHost());

  Client client(req.getPort().c_str(), req.getHost().c_str());

  if (cache_c.inCache(req,req_id))
  {
    Response *res = cache_c.getResponseFromCache(req, client.socket_fd, req_id);
    if (res == NULL)
    {
      //putError("fail to get response from cache");
      printError(req_id,"fail to get response from cache");
      return nullptr;
    }


    int status = send(client_fd, res->getResponse().c_str(), res->getResponse().length(), 0);
    if (status < 0)
    {
      //putError("fail to send response from cache to client");
      printError(req_id,"fail to send response from cache to client");
      return nullptr;
    }

    //outMessage(std::to_string(req_id)+"response sent from cache to client"+std::string(res->getStatus()));
    //close(client_fd);
    outRawMessage(std::to_string(req_id)+": response sent from cache to client");
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

try{
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


  //outMessage(std::to_string(req_id)+"response sent from server to client"+std::string(final_res.getStatus()));
  outRawMessage(std::to_string(req_id)+": Responding \""+final_res.getResponseLine()+"\"");
  if(final_res.getChunked()==false){
    pthread_mutex_lock(&cache_lock);
    if(
    cache_c.cacheRec(final_res, req, req_id)){
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
      }else{
    printNote(req_id,"cache failed because chunked");
  }
  //outMessage("after cache");
  //close(client_fd);
  return nullptr;

  }catch(std::exception e){
    //putError("exception in sendGET");
    printError(req_id,"exception in sendGET");
    return nullptr;
  }
  }catch(std::exception e){
    //putError("exception in sendGET");
    printError(req_id,"exception in sendGET:" + std::string(e.what()));
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

  try{

  Request reqPtr(req_get, req_id);

  std::time_t ctime = std::time(nullptr);
  std::tm* ctime_utc = std::gmtime(&ctime);
  std::string ctime_str = std::asctime(ctime_utc);

  outRawMessage(std::to_string(req_id)+": \""+reqPtr.getRequestLine()+"\" from "+reqPtr.getHost()+" @ "+getFirstLine(ctime_str));

  if(reqPtr.getMethod()=="GET"){
    sendGET(reqPtr, client_fd, req_id);
  }
  if(reqPtr.getMethod()=="POST"){
    sendPOST(reqPtr, client_fd, req_id);
  }
  if(reqPtr.getMethod()=="CONNECT"){
    sendCONNECT(reqPtr, client_fd, req_id);
    outRawMessage(std::to_string(req_id)+": Tunnel closed");
  }

  if(reqPtr.getMethod()!="GET" && reqPtr.getMethod()!="POST" && reqPtr.getMethod()!="CONNECT"){
    error400(client_fd, req_id);
  }
  }catch(std::exception &e){
    printError(req_id,"Exception: "+std::string(e.what()));
    return nullptr;
  }

  close(client_fd);
  //outMessage("test done"+std::to_string(req_id));
  printNote(req_id,"thread done");
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
    //std::cout << "Daemon process created with pid: " << m_pid << std::endl;
    std::cout<<"First fork success: "<<m_pid<<std::endl;
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
  try{
  serverP.initServer();
  int req_id = -1;
  int client_fd;
  std::string client_ip;
  outMessage("Proxy started");
  while(true){
    try{
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
    }catch(std::exception &e){
      outRawMessage("Error: exception in mainProcess: "+std::string(e.what()));
    }

  }
  }catch(std::exception &e){
    outRawMessage("Error: exception in mainProcess: "+std::string(e.what()));
    mainProcess();
  }
  
}


#endif