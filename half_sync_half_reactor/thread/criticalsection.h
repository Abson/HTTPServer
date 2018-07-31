#ifndef CIRTICALSECTION_H 
#define CIRTICALSECTION_H 

#include <pthread.h>

#if defined (__clang__) && (!defined (SWIG))
#define THREAD_ANNOTATION_ATTRIBUTE__(x) __attribute__((x))
#else
#define THREAD_ANNOTATION_ATTRIBUTE__(x) 
#endif

#define SCOPE_LOCKABLE THREAD_ANNOTATION_ATTRIBUTE__(scoped_lockable)

#define EXCLUSIVE_LOCK_FUNCTION(...) \
  THREAD_ANNOTATION_ATTRIBUTE__(exclusive_lock_function(__VA_ARGS__))

#define UNLOCK_FUNCTION(...) \
  THREAD_ANNOTATION_ATTRIBUTE__(unlock_function(__VA_ARGS__))

#define EXCLUSIVE_TRYLOCK_FUNCTION(...) \
  THREAD_ANNOTATION_ATTRIBUTE__(exclusive_trylock_function(__VA_ARGS__))

class CriticalSection {
public:
  CriticalSection();
  ~CriticalSection();

  void Enter() const EXCLUSIVE_LOCK_FUNCTION();
  bool TryEnter() const EXCLUSIVE_TRYLOCK_FUNCTION(true);
  void Leave() const UNLOCK_FUNCTION();
private:
  bool CurrentThreadIsOwner() const;
  mutable pthread_mutex_t mutex_;
  mutable pthread_t thread_;
  mutable int recursion_count_;
};

class SCOPE_LOCKABLE CritScope {
public:
  explicit CritScope(const CriticalSection* cs) EXCLUSIVE_LOCK_FUNCTION(cs);
  ~CritScope() UNLOCK_FUNCTION();

private:
  const CriticalSection* const cs_;

  CritScope(const CritScope&) = delete;
  void operator=(const CritScope&) = delete;
};

class TryCritScope {
public:
  explicit TryCritScope(const CriticalSection* cs) EXCLUSIVE_LOCK_FUNCTION(cs);
  ~TryCritScope();

  bool locked() const __attribute__((__warn_unused_result__));

private:
  const CriticalSection* const cs_;
  const bool locked_;
  mutable bool lock_was_called;

  TryCritScope(const CritScope&) = delete;
  void operator=(const TryCritScope&) = delete;
};

#endif
