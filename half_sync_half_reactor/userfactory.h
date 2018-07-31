#ifndef USERFACTORY_H
#define USERFACTORY_H

#include <map>
#include <memory>
#include "http_conn.h"
#include "thread/criticalsection.h"
#include "commont/constructormagic.h"
#include <stdio.h>

template <typename Key>
class Keyable 
{
public:
  virtual void set_key(Key key) = 0;
  virtual Key key() const = 0;
  virtual ~Keyable() {}
protected:
  Key key_;
};

template <typename Key, typename T>
class UserFactory : public std::enable_shared_from_this<UserFactory<Key, T>>
{
public:
  /*单例模式，no-thread-safe*/
  static std::shared_ptr<UserFactory<Key, T>> Create() {
    if (!instance_) {
      instance_ = std::shared_ptr<UserFactory> (new UserFactory);
    }
    return instance_;
  }

  std::shared_ptr<T> Get(const Key& key);

private:
  UserFactory() = default; // 让开发者必须使用 create 方法创建

  BASE_DISALLOW_COPY_AND_ASSIGN(UserFactory);

private:
  static void DeleteUserCallback(const std::weak_ptr<UserFactory>&weak_userfactory, T* user);

  /*外层没有必要调用 remove，因为套接字会重用*/
  void RemoveUser(T* user);

private:
  static std::shared_ptr<UserFactory<Key, T>> instance_;

  CriticalSection lock_;
  std::map<Key, std::shared_ptr<T>> users_;
};

template <typename Key, typename T>
std::shared_ptr<UserFactory<Key, T>> UserFactory<Key, T>::instance_ = nullptr;

template <typename Key, typename T>
std::shared_ptr<T> UserFactory<Key, T>::Get(const Key& key)
{
  CritScope cs(&lock_); // 线程安全
  std::shared_ptr<T>& user = users_[key];
  if (!user)
  {
    // shared_from_this() 可以使引用计数器加1，防止 factory 先于 user 构析导致 core dump.
    // 必须强制把 shared_from_this() 转型为 weak_ptr, 才不会延长生命期.
    user = std::shared_ptr<T>(new T, std::bind(&UserFactory::DeleteUserCallback,
          std::weak_ptr<UserFactory>( this->shared_from_this() ),
          std::placeholders::_1));

    user->set_key(key);
  }
  return user;
}

template <typename Key, typename T>
void UserFactory<Key, T>::DeleteUserCallback(const std::weak_ptr<UserFactory>&weak_userfactory, T* user)
{
  std::cout << __func__ << ": delete user " << user << " key " << user->key() << std::endl;
  std::shared_ptr<UserFactory> factory(weak_userfactory.lock());
  if (factory) 
  {
    factory->RemoveUser(user);
  }
  delete user;
}

template <typename Key, typename T>
void UserFactory<Key, T>::RemoveUser(T* user) 
{
  if (user)
  {
    CritScope cs(&lock_); // 注意这里会存在资源竞争问题
    users_.erase(user->key());
  }
}

#endif
