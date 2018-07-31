#include "http_conn.h"
#include "commont/commont.h"
#include <sys/mman.h>
#include "commont/stringutils.h"

const char* doc_root = "/home/parallels/Desktop";

const char* error_400_title = "Bad Request";
const char* error_400_form = "Your request has bad syntax or is inherently impossible to satisfy.\n";

const char* error_500_title = "Internal Error";
const char* error_500_form = "There was an unusual problem serving the requested file.\n";

const char* error_404_title = "Not Found";
const char* error_404_form = "The requested file was not found on this server.\n";

const char* error_403_title = "Forbidden";
const char* error_403_form = "You do not have permission to get file from this server.\n";

const char* ok_200_title = "OK";

int HTTPConn::epoll_fd_ = -1;
int HTTPConn::user_count_ = 0;

HTTPConn::HTTPConn() {}
HTTPConn::~HTTPConn() {}

void HTTPConn::Init(int sock_fd, const SocketAddress& addr) 
{
  sock_fd_ = sock_fd;
  addr_ = addr;
  /*如下两行是为了避免 TIME_WAIT 状态，仅用于调试，实际使用时应该去掉*/
  int reuse = 1;
  setsockopt(sock_fd_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

  base::AddFd(epoll_fd_, sock_fd_, true);
  user_count_++;
  Init();
}

void HTTPConn::Init() {
  read_idx_ = 0;
  checked_idx_ = 0;
  start_line_ = 0;
  write_idx_ = 0;
  check_state_ = CHECK_STATE::REQUESTING;
  method_ = GET;
  url_ = 0;
  version_ = 0; 
  host_ = "";
  content_length_ = 0;
  keeplive_ = false;
  file_address_ = nullptr;

  memset(read_buf_, '\0', READ_BUFFER_SIZE);
  memset(write_buf_, '\0', WRITE_BUFFER_SIZE);
  memset(real_file_, '\0', FILENAME_LEN);
}

/*循环读取客户数据，直到无数据可读或者对方关闭连接*/
bool HTTPConn::Read() {
  if (read_idx_ >= READ_BUFFER_SIZE) {
    return false;
  }

  while (true) {
    int bytes_read = recv(sock_fd_, read_buf_ + read_idx_,
        READ_BUFFER_SIZE - read_idx_, 0);
    if (bytes_read == 0) // 没有可读数据了
    { 
      return false;
    }
    else if (bytes_read == -1) 
    {
      if (errno == EAGAIN || errno == EWOULDBLOCK) /*数据读完了或操作将阻塞(可能跟没有数据可读一样)*/
      { 
        break;
      }
      return false;
    }
    
    read_idx_ += bytes_read;
  }
  return true;
}

HTTPConn::LINE_STATUS HTTPConn::ParseLine() {
  for (; checked_idx_ < read_idx_; ++checked_idx_) {

    char temp = read_buf_[checked_idx_];

    if (temp == '\r') {
      if ((checked_idx_ + 1) == read_idx_) {
        return LINE_OPEN;
      }
      else if (read_buf_[checked_idx_ + 1] == '\n') {
        read_buf_[checked_idx_++] = '\0';
        read_buf_[checked_idx_++] = '\0';
        return LINE_OK;
      }
      return LINE_BAD;
    }
    else if (temp == '\n') {
      if (checked_idx_ > 1 && read_buf_[checked_idx_ - 1] == '\r') {
        read_buf_[checked_idx_-1] = '\0';/*替换\r*/
        read_buf_[checked_idx_++] = '\0';
        return LINE_OK;
      }
      return LINE_BAD;
    }
  }
  return LINE_OPEN;
}

/*eg: "GET http://www.baidu.com/index.html HTTP/1.1"*/
HTTPConn::HTTP_CODE HTTPConn::ParseRequestLine(char* text) {
  std::cout << __func__ << std::endl;
  /*获取请求行*/
  url_ = strpbrk(text, " ");
  if (!url_) {
    return BAD_REQUEST;
  }
  *url_++ = '\0'; // 替换 "\t" 字符
  char* method = text;
  /*目前仅支持GET*/
  if (strcasecmp(method, "GET") == 0) {
    method_ = METHOD::GET;
  }
  else {
    return BAD_REQUEST;
  }

  url_ += strspn(url_, " "); // 跳过url_前多少个字符为空格"\t"
  version_ = strpbrk(url_, " ");// 查找url_ 中 "\t" 位置开始的字符串,得到HTTP/1.0
  if (!version_) {
    return BAD_REQUEST;
  }
  *version_++ = '\0';
  if (strcasecmp(version_, "HTTP/1.1") != 0) {
    return BAD_REQUEST;
  }
  // strncasecmp 函数比较前 url_ 前7个字符，是否与 http:// 相同
  if (strncasecmp(url_, "http://", 7) == 0) {
    url_ += 7;
    // strchr 返回第一个指向字符 / 的位置字符串, 就是文件名, index.html
    url_ = strchr(url_, '/');
  }
  if (!url_ || url_[0] != '/') {
    return BAD_REQUEST;
  }
  /*更改 check_state_，去解析请求头*/
  check_state_ = CHECK_STATE::HEADER;
  return NO_REQUEST;
}

/*eg: 
  "User-Agent:Wget/1.12(linux-gnu)
  Host:www.baidu.com
  Connection:close"
*/
HTTPConn::HTTP_CODE HTTPConn::ParseHeaders(char* text) {
  std::cout << __func__ << std::endl;
  /*遇到空行，表示头部字段解析完毕*/
  if (text[0] == '\0') {
    if (content_length_ != 0) {
        /*如果HTTP请求有消息体, 则还需要读取 content_length 字节的消息体, 状态机转移到CHECK_STATE_CONTENT状态*/
      check_state_ = CHECK_STATE::CONNENT;
      return NO_REQUEST;
    }
    return GET_REQUEST;
  }
  else if ( strncasecmp(text, "Connection:", 11) == 0) {
    text += 11;
    text += strspn(text, " ");
    if (strcasecmp(text, "keep-alive") == 0) {
      keeplive_ = true;
    }
  }
  else if (strncasecmp(text, "Content-Length:", 15) == 0) {
    text += 15;
    text += strspn(text, " ");
    content_length_ = atoi(text);
  }
  else if (strncasecmp(text, "Host:", 5) == 0) {
    text += 5;
    text += strspn(text, " ");
    host_ = text;
  }
  else {
    std::cout << "oop! unknow headers" << text << std::endl;
  }
  return NO_REQUEST;
}

/*我们没有真正解析HTTP请求的消息体, 只是判断它是否被完整地读入了*/
HTTPConn::HTTP_CODE HTTPConn::ParseContent(char* text) {
  /*recv 方法接受到的字符串是否大于 内容长度 + 前面已解析的字节长度*/
  if (read_idx_ >= (content_length_ + checked_idx_)) {
    text[content_length_] = '\0';
    return GET_REQUEST;
  }
  return NO_REQUEST;
}

/*当得到一个完整的，正确的HTTP请求时，我们就分析目标文件的属性。如果目标文件存在、对所有用户可读，
 *且不是目录，则使用 mmap 将其映射到内存地址 file_address_ 处，并告诉调用者获取文件成功*/
HTTPConn::HTTP_CODE HTTPConn::DoRequest() 
{
  std::cout << __func__ << std::endl;
  strcpy(real_file_, doc_root);
  int len = strlen(real_file_);
  strncpy(real_file_ + len, url_, FILENAME_LEN-len-1);
  /*获取不到文件信息*/
  if (stat(real_file_, &file_stat_) < 0) 
  {
    return NO_REQUEST;
  }
  if (!(file_stat_.st_mode & S_IROTH))
  { // 是否有读取权限
    return FORBIDDEN_REQUEST;
  }
  if (S_ISDIR(file_stat_.st_mode))
  {
    return BAD_REQUEST;
  }
  int fd = open(real_file_, O_RDONLY); // 只读
  file_address_ = (char*)mmap(0, file_stat_.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
  close(fd);
  return FILE_REQUEST;
}

HTTPConn::HTTP_CODE HTTPConn::ProcessRead() {
  LINE_STATUS line_status = LINE_OK;
  HTTP_CODE ret = HTTP_CODE::NO_REQUEST;

  while ( (check_state_ == CHECK_STATE::CONNENT && line_status == LINE_OK)
      || (line_status = ParseLine()) == LINE_OK ) {

    /*一行一行的解析 HTTP 协议*/
    char* text = GetLine();
    start_line_ = checked_idx_;
    
    std::cout << "got 1 http line " << text << std::endl;
    std::cout << "current check_state:" << check_state_ << std::endl;
    switch (check_state_) {
      case CHECK_STATE::REQUESTING:
        {
          ret = ParseRequestLine(text);
          if (ret == HTTP_CODE::BAD_REQUEST) {
            return HTTP_CODE::BAD_REQUEST;
          }
          break; /*解析了请求行,继续获取下一行内容进行解析*/
        }
      case CHECK_STATE::HEADER:
        {
          ret = ParseHeaders(text); 
          if (ret == HTTPConn::BAD_REQUEST) {
            return HTTP_CODE::BAD_REQUEST;
          }
          else if (ret == GET_REQUEST) {
            return DoRequest(); /*解析最后一步了，直接返回*/
          }
          /*ret 状态没有改变，不断解析头部信息*/
          break;
        }
      case CHECK_STATE::CONNENT:
        {
          ret = ParseContent(text);
          if (ret == GET_REQUEST) {
            return DoRequest();
          }
          line_status = LINE_OPEN;
          break;
        }
      default:
        return INTERNAL_ERROR;
    }
  }

  return NO_REQUEST;
}

void HTTPConn::Unmap() 
{
  if (file_address_) {
    munmap(file_address_, file_stat_.st_size);
    file_address_ = nullptr;
  }
}

bool HTTPConn::AddStatusLine(int status, const char* title) 
{
  return AddResponse("%s %d %s\r\n", "HTTP/1.1", status, title);
}

bool HTTPConn::AddHeaders(int content_length)
{
  bool ret = AddContentLength(content_length);
  if (!ret) { return false; }

  ret = AddKeepLive();
  if (!ret) { return false; }

  ret = AddBlankLine();
  return ret;
}

bool HTTPConn::AddContent(const char* content) 
{
  return AddResponse("%s", content);
}

bool HTTPConn::AddContentLength(int content_length)
{
  return AddResponse("Content-Length:%d\r\n", content_length);
}

bool HTTPConn::AddKeepLive() 
{
  return AddResponse("Connection:%s\r\n", (keeplive_ == true) ? "keep-alive":"close");
}

bool HTTPConn::AddBlankLine() 
{
  return AddResponse("%s", "\r\n");
}

bool HTTPConn::AddResponse(const char* format, ...) 
{
  if (write_idx_ >= WRITE_BUFFER_SIZE) {
    return false;
  }

  // int len = sprintfn(write_buf_ + write_idx_, WRITE_BUFFER_SIZE - 1 - write_idx_, format);
  va_list args;
  va_start(args, format);
  size_t len = vsprintfn(write_buf_ + write_idx_, WRITE_BUFFER_SIZE - 1 - write_idx_, format, args);
  va_end(args);

  if (len >= WRITE_BUFFER_SIZE - 1 - write_idx_) {
    return false;
  }
  write_idx_ += len;
  return true;
}

bool HTTPConn::Write() 
{
  int byte_have_send = 0; /*已经发送的字节数*/
  int byte_to_send = write_idx_; /*需要发送的字节数*/

  /*没有内容发送，返回*/
  if (byte_to_send == 0) {
    base::ModifyFd(epoll_fd_, sock_fd_, EPOLLIN);
    Init();
    return true;
  }

  while(1) {
    // 将 iv_ 中两个流合成一个流，写到 sock_fd_ 中
    int temp = writev(sock_fd_, iv_, iv_count_);
    if (temp <= -1) {
       /*如果TCP写缓冲没有空间, 则等待下一轮 EPOLLOUT 事件。虽然在此期间, 服务器无法立即接收到同一客户的下一个请求, 但这可以保证连接的完整性*/
      if (errno == EAGAIN) {
        base::ModifyFd(epoll_fd_, sock_fd_, EPOLLOUT);
        return true;
      }
     
      Unmap();
      return false;
    }
    
    printf("%s server write bytes to sock_fd:%d\n %s\n", __func__, sock_fd_, write_buf_);
    
    // byte_to_send -= temp;
    byte_have_send += temp;

    if (byte_to_send <= byte_have_send) {
      /*发送HTTP响应成功, 根据HTTP请求中的 Connection 字段决定是否立即关闭连接*/ 
      Unmap(); // 关闭内存映射
      if (keeplive_) 
      {
        Init();
        base::ModifyFd(epoll_fd_, sock_fd_, EPOLLIN);
        return true;
      }
      else 
      {
        base::ModifyFd(epoll_fd_, sock_fd_, EPOLLIN);
        return false;
      }
    }
  }
}

/*根据服务器处理HTTP请求的结果，决定返回给客户端的内容*/
bool HTTPConn::ProcessWrite(HTTPConn::HTTP_CODE code) 
{
  switch (code) {
    case HTTP_CODE::BAD_REQUEST :
      {
        AddStatusLine(400, error_400_title);
        AddHeaders(strlen(error_400_form));
        if (!AddContent(error_400_form)) {
          return false;
        }
        break;
      }
    case HTTP_CODE::INTERNAL_ERROR:
      {
        AddStatusLine(500, error_500_title);
        AddHeaders(strlen(error_500_form));
        if (!AddContent(error_500_form)) {
          return false;
        }
        break;
      }
    case HTTP_CODE::NO_REQUEST : 
      {
        AddStatusLine(404, error_404_title);
        AddHeaders(strlen(error_404_form));
        if (!AddContent(error_404_form)) {
          return false;
        }
        break;
      }
    case HTTP_CODE::FORBIDDEN_REQUEST :
      {
        AddStatusLine(403, error_403_title);
        AddHeaders(strlen(error_403_form));
        if (!AddContent(error_403_form)) {
          return false;
        }
        break;
      }
    case HTTP_CODE::FILE_REQUEST :
      {
        AddStatusLine(200, ok_200_title);
        if (file_stat_.st_size != 0) {
          AddHeaders(file_stat_.st_size);
          
          /*合并 write_buf_ 和 file_address 两个流*/
          iv_[0].iov_base = write_buf_;
          iv_[0].iov_len = write_idx_;
          iv_[1].iov_base = file_address_;
          iv_[1].iov_len = file_stat_.st_size;
          iv_count_ = 2;
          return true;
        }
        else {
          const char* ok_string = "<html><body></body></html>";
          AddHeaders(strlen(ok_string));
          if (!AddContent(ok_string)) {
            return false;
          }
        }
      }
    default:
      {
        return false;
      }
  }
  /*没有文件内容需要写入*/
  iv_[0].iov_base = write_buf_;
  iv_[0].iov_len = write_idx_;
  iv_count_ = 1;
  return true;
}

/*当线程池收到事件启动Set的时候，调用 Process*/
void HTTPConn::Process() 
{
  HTTP_CODE read_ret = ProcessRead();
  std::cout << "get read_ret: " << read_ret << std::endl;
  if (read_ret == NO_REQUEST) {
    base::ModifyFd(epoll_fd_, sock_fd_, EPOLLIN);
    return;
  }
  /*通过 ProcessRead 解析完HTTP协议信息后，通过 ProcessWrite 给客户
   * 发送返回信息*/
  bool write_ret = ProcessWrite(read_ret);
  if (!write_ret) {
    CloseConn();
  }
  base::ModifyFd(epoll_fd_, sock_fd_, EPOLLOUT);
}

void HTTPConn::CloseConn(bool real_close) 
{
  if (real_close && sock_fd_ != -1) 
  {
    base::RemoveFd(epoll_fd_, sock_fd_);
    sock_fd_ = -1;
    user_count_ --;
  }
}

