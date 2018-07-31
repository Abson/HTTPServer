#ifndef BASE_LOGGGING_H
#define BASE_LOGGGING_H

#include <string.h>
#include <functional>
#include "timestamp.h"
#include "logstream.h"

namespace base
{

class TimeZone;

class Logger 
{
public:
  enum LogLevel
  {
    TRACE,
    DEBUG,
    INFO,
    WARN,
    ERROR,
    FATAL,
    NUM_LOG_LEVELS,
  };

  class SourceFile 
  {
  public:
    template<int N>
    inline SourceFile(const char(&arr)[N])  /*const char (&arr)[N] C 字符串的引用*/
      : data_(arr), 
      size_(N-1)
    {
      /*查找到字符 '/' 后面的字符串才是真正的文件*/
      const char* slash = strrchr(data_, '/');
      if (slash) 
      {
        data_ = slash + 1;
        size_ -= static_cast<int>(data_ - arr);
      }
    }

    explicit SourceFile(const char* filename) 
      : data_(filename) 
    {
      /*查找到字符 '/' 后面的字符串才是真正的文件*/
      const char* slash = strrchr(data_, '/');
      if (slash) 
      {
        data_ = slash + 1;
      }
      size_ = static_cast<int>(strlen(data_));
    }

    const char* data_;
    int size_;
  };

  Logger(SourceFile file, int line);
  Logger(SourceFile file, int line, LogLevel level);
  Logger(SourceFile file, int line, LogLevel level, const char* func);
  Logger(SourceFile file, int line, bool toAbort);
  ~Logger();

  LogStream& stream() { return impl_.stream_; }

  static LogLevel loglevel();
  static void setLogLevel(LogLevel level);

  typedef std::function<void(const char* msg, int len)> OutputFunc;
  typedef std::function<void()> FlushFunc;
  static void setOutput(OutputFunc);
  static void setFlush(FlushFunc);
  static void setTimezone(const TimeZone& tz);

private:

  class Impl
  {
  public:
    typedef Logger::LogLevel LogLevel;
    Impl(LogLevel level, int old_errno, const SourceFile& file, int line);
    void formatTime();
    void finish();

    Timestamp time_;
    LogStream stream_;
    LogLevel level_;
    int line_;
    SourceFile basename_;
  };

  Impl impl_;
};

extern Logger::LogLevel g_logLevel;

inline Logger::LogLevel Logger::loglevel()
{
  return  g_logLevel;
}

#define LOG_TRACE if (base::Logger::loglevel() <= base::Logger::LogLevel::TRACE) \
  base::Logger(__FILE__, __LINE__, base::Logger::LogLevel::TRACE, __func__).stream()

#define LOG_INFO if (base::Logger::loglevel() <= base::Logger::LogLevel::INFO) \
  base::Logger(__FILE__, __LINE__).stream()

#define LOG_DEBUG if (base::Logger::loglevel() <= base::Logger::LogLevel::DEBUG) \
  base::Logger(__FILE__, __LINE__, base::Logger::LogLevel::DEBUG, __func__).stream()

#define LOG_WARN base::Logger(__FILE__, __LINE__, base::Logger::LogLevel::WARN).stream()

#define LOG_ERROR base::Logger(__FILE__, __LINE__, base::Logger::LogLevel::ERROR).stream()

#define LOG_FATAL base::Logger(__FILE__, __LINE__, base::Logger::LogLevel::FATAL).stream()

#define LOG_SYSERR base::Logger(__FILE__, __LINE__, false).stream()
#define LOG_SYSFATAL base::Logger(__FILE__, __LINE__, true).stream()

template <typename T>
T* CheckNotNull(Logger::SourceFile file, int line, const char* names, T* ptr)
{
  if (ptr == nullptr)
  {
    Logger(file, line, Logger::FATAL).stream() << names;
  }
}

}

#endif
