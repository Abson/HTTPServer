#include "http_conn.h"
#include "commont/commont.h"
#include "userfactory.h"

#define MAX_FD 0x10000
#define MAX_EVENT_NUMBER 10000

extern bool HandleListen(int listen_fd);

extern bool HandleReadConnfd(int connfd);

extern bool HandleWriteConnfd(int connfd);

/*预先未每个可能的客户连接分配一个 HTTPConn 对象*/
//std::vector<std::shared_ptr<HTTPConn>> users(MAX_FD, std::make_shared<HTTPConn>());

std::shared_ptr<UserFactory<int, HTTPConn>> users(UserFactory<int, HTTPConn>::Create());

std::unique_ptr<ThreadPool<HTTPConn>> pool = nullptr;

int main(int argc, const char* argv[]) 
{
  if (argc <= 2) {
    printf("usage:%s ipaddress port_number\n", argv[0]);
    return 1;
  }
  const char* ip = argv[1];
  int port = atoi(argv[2]);

  /*忽略SIGPIPE信号*/
  base::AddSig(SIGPIPE, SIG_IGN);

  try 
  {
    pool.reset(new ThreadPool<HTTPConn>());
  }
  catch (...)
  {
    return 1;
  }

  int listen_fd = socket(PF_INET, SOCK_STREAM, 0);
  assert(listen_fd);
 
  /* close() 后立刻返回，但不会发送未发送完成的数据，而是通过一个REST包强制的关闭socket描述符，即强制退出。
   * */
  struct linger temp = {1, 0};
  setsockopt(listen_fd, SOL_SOCKET, SO_LINGER, &temp, sizeof(temp));

  sockaddr_in sock_addr;
  SocketAddress addr = SocketAddress(std::string(ip), port);
  addr.ToSockAddr(&sock_addr);
  int ret = bind(listen_fd, (struct sockaddr*)&sock_addr, sizeof(sock_addr));
  assert(ret >= 0);

  ret = listen(listen_fd, 5);
  assert(ret >= 0);

  epoll_event events[MAX_EVENT_NUMBER];
  int epoll_fd = epoll_create(5);
  assert(epoll_fd != -1);

  base::AddFd(epoll_fd, listen_fd);
  HTTPConn::epoll_fd_ = epoll_fd;

  // event loop
  while(1) 
  {
    int number = epoll_wait(epoll_fd, events, MAX_EVENT_NUMBER, -1);
    if (number < 0 && errno != EINTR)
    {
      printf("epoll failure\n");
      break;
    }
    for (int i = 0; i < number; i++) 
    {
      int sock_fd = events[i].data.fd;

      if (sock_fd == listen_fd && events[i].events & EPOLLIN) 
      {
        bool ret = HandleListen(listen_fd);
        if (!ret) { continue; }
      }
      else if (events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR))
      {
        /*发生异常，直接关闭客户连接*/
        users->Get(sock_fd)->CloseConn(true);
      }
      else if (events[i].events & EPOLLIN) 
      {
        HandleReadConnfd(sock_fd);
      }
      else if (events[i].events & EPOLLOUT)
      {
        HandleWriteConnfd(sock_fd);
      }
      else{
        std::cout << "get some havent hanndle epoll event" << std::endl;
      }
    }
  }

  base::RemoveFd(epoll_fd, listen_fd);
  close(epoll_fd);
  return 0;
}

bool HandleListen(int listen_fd) 
{
  sockaddr_in client_addr;
  socklen_t len = sizeof(client_addr);
  int connfd = accept(listen_fd, (struct sockaddr*)&client_addr, &len);
  if (connfd < 0) 
  {
    printf("errno is: %d\n", errno);
    return false;
  }
  if (HTTPConn::user_count_ >= MAX_FD) 
  {
    base::ShowError(connfd, "Internal server busy");
    return false;
  }
  SocketAddress addr;
  addr.FromSockAddr(client_addr);
  /*初始化客户连接*/
  users->Get(connfd)->Init(connfd, addr);
  return true;
}

bool HandleReadConnfd(int connfd) 
{
  /*根据读的结果, 决定是将任务添加到线程池, 还是关闭连接*/
  bool ret = users->Get(connfd)->Read(); // 获取客户端发送的字节流
  printf("%s return %d \n", __func__, ret);
  if (ret)
  {
    pool->append(users->Get(connfd)); // 通过线程池，派发到子线程当中去处理读操作
    return true;
  }
  else 
  {
    users->Get(connfd)->CloseConn(true);
    return false;
  }
}

bool HandleWriteConnfd(int connfd)
{
  bool ret = users->Get(connfd)->Write();
  printf("%s return %d \n", __func__, ret);
  if (!ret)
  {
    users->Get(connfd)->CloseConn(true);
    return true;
  }
  return false;
}
