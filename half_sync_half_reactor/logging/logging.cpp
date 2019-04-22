#include "logging.h"

#include "logstream.h"
#include "timezone.h"
#include "current_thread.h"

#include <assert.h>
#include <string.h>

namespace base
{

__thread char t_errnobuf[512];
__thread char t_time[64];
__thread time_t t_lastSecond;

const char* strerror_tl(int savedErrno)
{
  return strerror_r(savedErrno, t_errnobuf, sizeof(t_errnobuf));
}

Logger::LogLevel initLogLevel()
{ 
  if (::getenv("BASE_LOG_TRACE"))
    return Logger::TRACE;
  else if (::getenv("BASE_LOG_DEBUG"))
    return Logger::DEBUG;
  else 
    return Logger::INFO;
}

Logger::LogLevel g_logLevel = initLogLevel();

const char* LogLevelName[Logger::NUM_LOG_LEVELS] =
{
  "TRACE ",
  "DEBUG ",
  "INFO  ",
  "WARN  ",
  "ERROR ",
  "FATAL ",
};

class T
{
public:
  T(const char* str, unsigned len) 
    : str_(str),
      len_(len)
  {
    assert(strlen(str) == len_);
  }

  const char* str_;
  const unsigned len_;
};

inline LogStream& operator<<(LogStream& s, T v)
{
  s.append(v.str_, v.len_);
  return s;
}

inline LogStream& operator<<(LogStream& s, const Logger::SourceFile& v)
{
  s.append(v.data_, v.size_);
  return s;
}

void defaultOutput(const char* msg, int len)
{
  size_t n = fwrite(msg, 1, len, stdout);
  static_cast<void>(n);
}

void defaultFlush()
{
  fflush(stdout);
}

Logger::OutputFunc g_output = defaultOutput;
Logger::FlushFunc g_flush = defaultFlush;
TimeZone g_logTimeZone;

}

using namespace base;

Logger::Impl::Impl(Logger::LogLevel level, int savedErrno, const Logger::SourceFile& file, int line)
  : time_(Timestamp::now()),
    stream_(),
    level_(level),
    line_(line),
    basename_(file)
{
  formatTime();
  CurrentThread::tid();
  stream_ << T(CurrentThread::tidString(), CurrentThread::tidStringLength());
  stream_ << T(LogLevelName[level], 6);
  if (savedErrno != 0)
  {
    stream_ << strerror_tl(savedErrno) << " (errno=" << savedErrno << ") ";
  }
}

void Logger::Impl::formatTime()
{
  int64_t microSecondsSinceEpoch = time_.microSecondsSinceEpoch();
  /* 秒 */
  time_t seconds = static_cast<time_t>(microSecondsSinceEpoch / Timestamp::kMicroSecondsPerSecond);
  /* 毫秒 */
  int microseconds = static_cast<int>(microSecondsSinceEpoch % Timestamp::kMicroSecondsPerSecond);
  if (seconds != t_lastSecond)
  {
    t_lastSecond = seconds;

    struct tm tm_time;
    if (g_logTimeZone.valid())
    {
      tm_time = g_logTimeZone.toLocalTime(microSecondsSinceEpoch);
    }
    else 
    {
      ::gmtime_r(&seconds, &tm_time);
    }

    std::cout << tm_time.tm_year + 1900 << " " << tm_time.tm_mon + 1 << " "<< tm_time.tm_mday << " "<<
        tm_time.tm_hour << " " << tm_time.tm_min << " " << tm_time.tm_sec << std::endl;

    int len = snprintf(t_time, sizeof(t_time), "%4d%02d%02d %02d:%02d:%02d",
        tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
        tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec);
    std::cout << "t_time: " << t_time << " len: " << len << std::endl;
    assert(len == 17); (void)len;
    static_cast<void>(len);
  }

  /* if (g_logTimeZone.valid()) */
  /* { */
    Fmt us(".%06dZ ", microseconds);
    assert(us.length() == 9);
    stream_ << T(t_time, 17) << T(us.data(), 9);
  /* } */
}

void Logger::Impl::finish()
{
  stream_ << " - " << basename_ << ':' << line_ << '\n';
}

Logger::Logger(Logger::SourceFile file, int line) 
  : impl_(INFO, 0, file, line)
{}

Logger::Logger(Logger::SourceFile file, int line, Logger::LogLevel level, const char* func)
  : impl_(level, 0, file, line)
{
  impl_.stream_ << func << ' ';
}

Logger::Logger(Logger::SourceFile file, int line, Logger::LogLevel level)
  : impl_(level, 0, file, line)
{}

Logger::Logger(Logger::SourceFile file, int line, bool toAbort)
  : impl_(toAbort ? FATAL : ERROR, errno, file, line)
{}

Logger::~Logger()
{
  impl_.finish();
  const LogStream::Buffer& buf(stream().buffer());
  g_output(buf.data(), buf.length());
}

void Logger::setLogLevel(Logger::LogLevel level)
{
  g_logLevel = level;
}

void Logger::setOutput(OutputFunc out)
{
  g_output = out;
}

void Logger::setFlush(FlushFunc flush)
{
  g_flush = flush;
}

void Logger::setTimezone(const TimeZone& tz)
{
  g_logTimeZone = tz;
}
