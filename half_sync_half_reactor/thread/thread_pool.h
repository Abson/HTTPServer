#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include "criticalsection.h"
#include "event.h"

#include <iostream>
#include <vector>
#include <list>
#include <memory>

class RequestHandlerInterface {
public:
  RequestHandlerInterface () {}
  virtual ~RequestHandlerInterface () {}

  virtual void Process() = 0;
  virtual bool Write() = 0;
  virtual bool Read() = 0;
};

template <typename T>
class ThreadPool {
public:
  ThreadPool(int thread_number = 8, int max_requests = 10000);
  ~ThreadPool();

  /*往请求对了中添加任务*/
  bool append(std::shared_ptr<T> request);

private:
  static void* Worker(void* arg);
  void Run();

private:
  int thread_number_;
  int max_requests_;
  std::unique_ptr<Event> event_;

  std::vector<pthread_t> threads_;
  std::list<std::shared_ptr<T>>work_queue_;
  CriticalSection queue_lock_;/*保护请求队列的互斥锁*/
  bool stop_; /*是否结束线程*/
};

template <typename T>
ThreadPool<T>::ThreadPool(int thread_number, int max_requests) :
  thread_number_(thread_number), max_requests_(max_requests), 
  stop_(false) 
{
    if (thread_number <= 0 || max_requests_ <= 0) {
      throw std::exception();
    }

    threads_ = std::vector<pthread_t>(thread_number_);
 
    event_.reset(new Event(false, false));

    for (int i = 0; i < thread_number_; i++) 
    {
      printf("create the %dth thread\n", i);
      pthread_t& tid = threads_[i];
      if (pthread_create(&tid, NULL, Worker, this) != 0) {
        throw std::exception();
      } 
      if (pthread_detach(tid)) {
        throw std::exception();
      }
    }

}

template <typename T>
ThreadPool<T>::~ThreadPool() {
  stop_ = true;
}

template <typename T>
bool ThreadPool<T>::append(std::shared_ptr<T> request) 
{
  /*操作工作队列一定要加锁*/
  {
    CritScope cs(&queue_lock_);
    if (work_queue_.size() > max_requests_) { 
      return false;
    }
    work_queue_.push_back(request);
  }
  event_->Set();
  return true;
} 

template <typename T>
void ThreadPool<T>::Run()
{
  // one loop per thread
  while(!stop_) 
  {
    event_->Wait(Event::kForever);
    std::shared_ptr<T> request;
    /*利用线程的抢占模式*/
    {
      CritScope cs(&queue_lock_);
      
      std::cout << "thread: " << pthread_self() << " start handle request!!!" << std::endl;

      if (work_queue_.empty()) 
        continue;

      request = work_queue_.front();
      work_queue_.pop_front();
    }

    if (!request) 
      continue;

    request->Process();
  }
}

/*多线程运行方法*/
template <typename T>
void* ThreadPool<T>::Worker(void* arg) {
  ThreadPool* pool = static_cast<ThreadPool*>(arg);
  pool->Run();
  return pool;
} 

#endif
