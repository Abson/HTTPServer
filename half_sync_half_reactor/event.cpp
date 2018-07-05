#include "event.h"
#include <assert.h>
#include <sys/time.h>

Event::Event(bool manual_reset, bool initially_signaled) :
  is_manual_reset_(manual_reset),
  event_status_(initially_signaled) {

    assert(pthread_mutex_init(&event_mutex_, nullptr) == 0);
    assert(pthread_cond_init(&event_cond_, nullptr) == 0);
}

Event::~Event() {
  pthread_mutex_destroy(&event_mutex_);
  pthread_cond_destroy(&event_cond_);
}

void Event::Set() {
  pthread_mutex_lock(&event_mutex_);
  event_status_ = true;
  pthread_cond_broadcast(&event_cond_);
  pthread_mutex_unlock(&event_mutex_);
}

void Event::Reset() {
  pthread_mutex_lock(&event_mutex_);
  event_status_ = false;
  pthread_mutex_unlock(&event_mutex_);
}

bool Event::Wait(int milliseconds) {
  int error = 0;

  struct timespec ts;

  if (milliseconds != kForever) {
    struct timeval tv;
    gettimeofday(&tv, nullptr);

    ts.tv_sec = tv.tv_sec + (milliseconds / 1000);
    ts.tv_nsec = tv.tv_usec + (milliseconds % 1000);

    // Handl overflow
    if (ts.tv_nsec >= 1000000000) {
      ts.tv_sec++;
      ts.tv_nsec -= 1000000000;
    }
  }

  pthread_mutex_lock(&event_mutex_);
  if (milliseconds != kForever) {
    while(!event_status_ && error == 0) {
      error = pthread_cond_timedwait(&event_cond_, &event_mutex_, &ts);
    }
  }
  else {
    while(!event_status_ && error == 0) {
      error = pthread_cond_wait(&event_cond_, &event_mutex_);
    }
  }

  if (error == 0 && is_manual_reset_) {
    event_status_ = false;
  }

  pthread_mutex_unlock(&event_mutex_);

  return (error == 0);
}
