#ifndef HTTP_CONNECTION_H
#define HTTP_CONNECTION_H 

#include "thread_pool.h"
#include "socketaddress.h"
#include <sys/stat.h>
#include "userfactory.h"

class HTTPConn : public RequestHandlerInterface, public Keyable<int>  {
public:
  HTTPConn();
  ~HTTPConn();

public:
  void Init(int sock_fd, const SocketAddress& addr);

  void Process() override;

  /*非阻塞读操作, 开发者先使用 Read 将用户请求字节读取，然后调用 process 去去处理字节*/
  bool Read() override;

  bool Write() override;

  void set_key(int key) override { key_ = key; }

  int key() const override { return key_; }

  void CloseConn(bool real_close = true);
public:
  /*文件名的最大长度*/
  static const int FILENAME_LEN = 200;
  /*读缓冲区的大小*/
  static const int READ_BUFFER_SIZE = 2048;
  /*写缓冲区的大小*/
  static const size_t WRITE_BUFFER_SIZE = 1024;

  /*HTTP请求方法，但我们仅支持Get*/
  enum METHOD{GET = 0, POST, HEAD, PUT, DELETE, OPTIONS, CONNECT, PATCH};
  /*解析客户请求时，主机状态所处的状态*/
  enum CHECK_STATE{REQUESTING = 0, HEADER, CONNENT};
  /*服务器处理HTTP请求可能的结果*/
  enum HTTP_CODE{NO_REQUEST, GET_REQUEST, BAD_REQUEST, 
    NO_RESOURCE, FORBIDDEN_REQUEST, FILE_REQUEST, INTERNAL_ERROR, CLOSE_CONNECTION};
  /*行的读取状态*/
  enum LINE_STATUS {LINE_OK = 0, LINE_BAD, LINE_OPEN};

public:

  static int epoll_fd_;
  static int user_count_;

private:
  void Init();

  HTTP_CODE ProcessRead();

  bool ProcessWrite(HTTP_CODE code);

  LINE_STATUS ParseLine();

  HTTP_CODE ParseRequestLine(char* text);

  HTTP_CODE ParseHeaders(char* text);

  HTTP_CODE ParseContent(char* text);

  HTTP_CODE DoRequest();

  bool AddStatusLine(int status, const char* title);

  bool AddHeaders(int content_length);

  bool AddContent(const char* content);

  bool AddContentLength(int content_length);

  bool AddKeepLive();

  bool AddBlankLine();

  bool AddResponse(const char* format, ...);

  char* GetLine() { return read_buf_ + start_line_; }

  void Unmap();
private:
  int sock_fd_;
  SocketAddress addr_;
  /*读缓冲区*/
  char read_buf_[READ_BUFFER_SIZE];
  /*标识读缓冲区已经读入的客户数据最后一个字节的下一个位置*/
  int read_idx_;
  /*当前正在分析的字符在读缓冲区中的位置*/
  int checked_idx_;
  /*当前正在解析行的起始位置*/
  int start_line_;
  /*写缓冲区*/
  char write_buf_[WRITE_BUFFER_SIZE];
  /*写缓冲区中待发送的字节数*/
  size_t write_idx_;
  /*主状态机当前所处状态*/
  int check_state_;
  /*请求方式*/
  METHOD method_;
  /*客户请求目标文件的完整路径*/
  char real_file_[FILENAME_LEN];
  /*客户请求的目标文件的文件名*/
  char* url_;
  /*HTTP 协议版本号，目标仅支持 HTTP/1.1*/
  char* version_;
  /*主机名*/
  std::string host_;
  /*HTTP请求的消息体长度*/
  int content_length_;
  /*HTTP请求是否要求保持连接*/
  bool keeplive_;
  /*客户请求的文件被mmap到内存中的起始位置*/
  char* file_address_;
  /*目标文件状态。通过它们我们可以判断文件是否存在，是否为目录，是否可读，并获取
   *文件大小等信息*/
  struct stat file_stat_;
  /*我们将采用 writev 来执行写操作，所以定义下面两个成员，其中 iv_count_ 表示被
   * 写入内存块的数量*/
  struct iovec iv_[2];
  int iv_count_;
};

#endif
