#include "logstream.h"

#include <assert.h>

#include <algorithm>

namespace base
{
namespace detail
{

const char digits[] = "9876543210123456789";
const char* zero = digits + 9;

const char digitsHex[] = "0123456789ABCDEF";

template<typename T>
size_t covert(char buf[], T value)
{
  T i = value;
  char* p = buf;
  /*从 value 中每个位转为  0-9 的字符串*/
  do 
  {
    int lsd = static_cast<int>( i % 10 );
    i /= 10;
    *p++ = zero[lsd];
  } while(i != 0);

  /*添加正负号*/
  if (value < 0)
  {
    *p++ = '-';
  }
  *p = '\0';
  /* 由于是从个位开始添加字符串的，需要反转容器元素顺序 */
  std::reverse(buf, p);

  return p - buf;
}

size_t convertHex(char buf[], uintptr_t value)
{
  uintptr_t i = value;
  char* p = buf;

  do 
  {
    int lsd = static_cast<int>(i % 16);
    i /= 16;
    *p++ = digitsHex[lsd];
  } while(i != 0);
  *p++ = '\0';
  std::reverse(buf, p);
  
  return p - buf;
}

template class FixedBuffer<kSmallBuffer>;
template class FixedBuffer<kLargeBuffer>;

}
}

using namespace base;
using namespace base::detail;

template <int SIZE>
const char* FixedBuffer<SIZE>::debugString()
{
  *cur_ = '\0';
  return data_;
}

template <int SIZE>
void FixedBuffer<SIZE>::cookieStart()
{
}

template <int SIZE>
void FixedBuffer<SIZE>::cookieEnd()
{
}

/* LogStream */
void LogStream::staticCheck()
{
}

template<typename T>
void LogStream::formatInteger(T v)
{
  if (buffer_.avail() >= kMaxNumericSize)
  {
    size_t len = covert(buffer_.current(), v);
    buffer_.add(len);
  }
}

LogStream& LogStream::operator<<(int v)
{
  formatInteger(v);
  return *this;
}

LogStream& LogStream::operator<<(unsigned int v)
{
  formatInteger(v);
  return *this;
}

LogStream& LogStream::operator<<(short v)
{
  *this << static_cast<int>(v);
  return *this;
}

LogStream& LogStream::operator<<(unsigned short v)
{
  *this << static_cast<int>(v);
  return *this;
}

LogStream& LogStream::operator<<(long v)
{
  formatInteger(v);
  return *this;
}

LogStream& LogStream::operator<<(unsigned long v)
{
  formatInteger(v);
  return *this;
}

LogStream& LogStream::operator<<(long long v)
{
  formatInteger(v);
  return *this;
}

LogStream& LogStream::operator<<(unsigned long long v)
{
  formatInteger(v);
  return *this;
}

LogStream& LogStream::operator<<(const void* v)
{
  uintptr_t p = reinterpret_cast<uintptr_t>(v);
  if (buffer_.avail() >= kMaxNumericSize)
  {
    char* buf = buffer_.current();
    buf[0] = '0';
    buf[1] = 'x';
    size_t len = convertHex(buf+2, p);
    buffer_.add(len);
  }
  return *this;
}

template<typename T>
Fmt::Fmt(const char* fmt, T val)
{
  length_ = snprintf(buf_, sizeof(buf_), fmt, val);
  assert(static_cast<size_t>(length_) < sizeof(buf_));
}

/* 显式实例化 */
template Fmt::Fmt(const char* fmt, char);
template Fmt::Fmt(const char* fmt, short);
template Fmt::Fmt(const char* fmt, unsigned short);
template Fmt::Fmt(const char* fmt, int);
template Fmt::Fmt(const char* fmt, unsigned int);
template Fmt::Fmt(const char* fmt, long);
template Fmt::Fmt(const char* fmt, unsigned long);
template Fmt::Fmt(const char* fmt, long long);
template Fmt::Fmt(const char* fmt, unsigned long long);

template Fmt::Fmt(const char* fmt, float);
template Fmt::Fmt(const char* fmt, double);


