#include "criticalsection.h"
#include <assert.h>

CriticalSection::CriticalSection() {
  pthread_mutexattr_t attr;
  pthread_mutexattr_init(&attr);
  /*PTHREAD_MUTEX_RECURSIVE 如果一个线程对这种类型的互斥锁重复上锁，不会引起死锁。
   * 一个线程对这类互斥锁的多次重复上锁必须由这个线程来重复相同数量的解锁，这样才能解开这个互斥锁，别的线程才能得到这个互斥锁。
   *如果试图解锁一个由别的线程锁定的互斥锁将会返回一个错误代码。如果一个线程试图解锁已经被解锁的互斥锁也将会返回一个错误代码。
   **/
  pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
  pthread_mutex_init(&mutex_, &attr);
  pthread_mutexattr_destroy(&attr);

  thread_ = 0;
  recursion_count_ = 0;
}

CriticalSection::~CriticalSection() {
  pthread_mutex_destroy(&mutex_);
}

bool CriticalSection::CurrentThreadIsOwner() const {
  return pthread_equal(thread_, pthread_self());
}

void CriticalSection::Enter() const {
  pthread_mutex_lock(&mutex_);

  if (!recursion_count_) {
    assert(!thread_);
    thread_ = pthread_self();
  }
  else {
    CurrentThreadIsOwner();
  }
  ++recursion_count_;
}

bool CriticalSection::TryEnter() const {
  if ( pthread_mutex_trylock(&mutex_) != 0)  {
    return false;
  }

  if (!recursion_count_) {
    assert(!thread_);
    thread_ = pthread_self();
  }
  else {
    CurrentThreadIsOwner();
  }
  ++recursion_count_;
  return true;
}

void CriticalSection::Leave() const {

  --recursion_count_;
  assert(recursion_count_ >= 0);
  if (!recursion_count_)
    thread_ = 0;

  pthread_mutex_unlock(&mutex_);
}

CritScope::CritScope(const CriticalSection* cs) : cs_(cs) {
  cs_->Enter();
}

CritScope::~CritScope() {
  cs_->Leave();
}

TryCritScope::TryCritScope(const CriticalSection* cs) : 
  cs_(cs), 
  locked_(cs_->TryEnter()) {
    lock_was_called = false;
  } 

bool TryCritScope::locked() const {
  lock_was_called = true;
  return locked_;
}

TryCritScope::~TryCritScope() {
  if (locked_) {
    cs_->Leave();
  }
}
