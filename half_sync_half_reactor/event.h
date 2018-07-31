#ifndef BASE_EVENT_H
#define BASE_EVENT_H

#include "commont/constructormagic.h"
#include <pthread.h>

class Event {
public:
  static const int kForever = -1;

  Event(bool manual_reset, bool initially_signaled);
  ~Event();

  void Set();
  void Reset();

  bool Wait(int milliseconds);

private:
  pthread_mutex_t event_mutex_;
  pthread_cond_t event_cond_;

  /*用户是否需要自行调用 Reset 方法 event_status_*/
  const bool is_manual_reset_;

  bool event_status_;

  BASE_DISALLOW_COPY_AND_ASSIGN(Event);
};

#endif
