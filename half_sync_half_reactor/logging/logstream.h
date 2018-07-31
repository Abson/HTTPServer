#ifndef BASE_LOGSTREAM_H
#define BASE_LOGSTREAM_H

#include "../commont/constructormagic.h"
#include <functional>
#include <string.h>

namespace base
{
namespace detail
{

const int kSmallBuffer = 4 * 1024;
const int kLargeBuffer = 4 * 1024 * 1024;

template<int SIZE>
class FixedBuffer
{
public:
  FixedBuffer()
    : cur_(data_)
  {
    setCookie(cookieEnd);
  }

  ~FixedBuffer()
  {
    setCookie(cookieEnd);
  }
  
  void setCookie(std::function<void()> func) 
  {
    cookie_ = func;
  }

  void append(const char* buf, size_t len)
  {
    if (static_cast<size_t>(avail() > len))
    {
      memcpy(cur_, buf, len);
      cur_ += len;
    }
  }

  void append(const std::string& buf)
  {
    append(buf.c_str(), buf.size());
  }

  const char* data() const { return data_; }
  int length() const { return static_cast<int>(cur_ - data_); }

  char* current() { return cur_; }
  int avail() const { return static_cast<int>(end() - cur_); }
  void add(size_t len) { cur_ += len; }

  void reset() { cur_ = data_; }
  void bzero() { ::bzero(data_, sizeof(data_)); }

  const char* debugString();

  std::string toString() const { return std::string(data_, length()); }

private:
  const char* end() const { return data_ + sizeof(data_); }
  static void cookieStart();
  static void cookieEnd();

  std::function<void()> cookie_;
  char data_[SIZE];
  char* cur_;

private:
  BASE_DISALLOW_ASSIGN(FixedBuffer);
};

}

class LogStream
{
  typedef LogStream self;
public:
  typedef detail::FixedBuffer<detail::kSmallBuffer> Buffer;

  self& operator<<(bool v)
  { 
    buffer_.append(v ? "1" : "0");
    return *this;
  }

  self& operator<<(short);
  self& operator<<(unsigned short);
  self& operator<<(int);
  self& operator<<(unsigned int);
  self& operator<<(long);
  self& operator<<(unsigned long);
  self& operator<<(long long);
  self& operator<<(unsigned long long);

  self& operator<<(const void*);

  self& operator<<(float v)
  {
    *this << static_cast<double>(v);
    return *this;
  }

  self& operator<<(double v);

  self& operator<<(char v)
  {
    buffer_.append(&v, 1);
    return *this;
  }

  self& operator<<(const char* str)
  {
    if (str)
    {
      buffer_.append(str, strlen(str));
      return *this;
    }
    else
    {
      buffer_.append("(null)");
    }
    return *this;
  }

  self& operator<<(const unsigned char* str)
  {
    return operator<<(reinterpret_cast<const char*>(str));
  }

  self& operator<<(const std::string& v)
  {
    buffer_.append(v);
    return *this;
  }

  void append(const char* data, int len) { buffer_.append(data, len); }
  const Buffer& buffer() const { return buffer_; }
  void resetBuffer() { buffer_.reset(); }

private:
  void staticCheck();

  template<typename T>
  void formatInteger(T);

  Buffer buffer_;

  static const int kMaxNumericSize = 32;
private:
  BASE_DISALLOW_ASSIGN(LogStream);
};

class Fmt
{
public:
  template<typename T>
  Fmt(const char* fmt, T val);

  const char* data() const { return buf_; }
  int length() const { return length_; }

private:
  char buf_[32];
  int length_;
};

inline LogStream& operator<<(LogStream& s, const Fmt& fmt)
{
  s.append(fmt.data(), fmt.length());
  return s;
}

}

#endif
