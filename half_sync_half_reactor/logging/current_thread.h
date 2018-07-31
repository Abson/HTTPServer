#ifndef BASE_CURRENTTHREAD_H
#define BASE_CURRENTTHREAD_H 

#include "timestamp.h"

#include <stdint.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <time.h>

namespace base
{
pid_t gettid()
{
  return static_cast<pid_t>(::syscall(SYS_gettid));
}

namespace CurrentThread
{

  __thread int t_cachedTid = 0;
  __thread char t_tidString[32];
  __thread int t_tidStringLength = 6;
  __thread const char* t_threadName = "unknown";

  void cacheTid()
  {
    if (t_cachedTid == 0)
    {
      t_cachedTid = gettid();
      t_tidStringLength = snprintf(t_tidString, sizeof(t_tidString), "%5d ", t_cachedTid);
    }
  }

  inline int tid()
  {
    if (__builtin_expect(t_cachedTid == 0, 0))
    {
      cacheTid();
    }
    return t_cachedTid;
  }

  inline const char* tidString()
  {
    return t_tidString;
  }

  inline int tidStringLength()
  {
    return t_tidStringLength;
  }

  inline const char* name()
  {
    return t_threadName;
  }

  bool isMainThread()
  {
    return tid() == ::getpid();
  }

  void sleepUsec(int64_t usec)
  {
    struct timespec ts = {0, 0};
    ts.tv_sec = static_cast<time_t>(usec / Timestamp::kMicroSecondsPerSecond);
    ts.tv_nsec = static_cast<time_t>(usec % Timestamp::kMicroSecondsPerSecond * 1000);
    ::nanosleep(&ts, nullptr);
  }
}
}

#endif
