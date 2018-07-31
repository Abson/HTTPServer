#ifndef COMMONT_H
#define COMMONT_H

#include <fcntl.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <assert.h>
#include <string.h>
#include <iostream>
#include <sys/socket.h>

namespace base {

  static int SetNoBlocking(int fd) {
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
  }

  static void AddFd(int epoll_fd, int fd, int one_shot = 0) {
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET | EPOLLHUP;
    if (one_shot) {
      event.events |= EPOLLONESHOT;
    }
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &event);
    SetNoBlocking(fd);
  }

  static void RemoveFd(int epoll_fd, int fd) {
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, 0);
    close(fd);
    std::cout << "remove sockf_fd: " << fd << std::endl;
  }

  static void AddSig(int sig, void(handler)(int), bool restart = true) {
    struct ::sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = handler;
    if ( restart ) {
      sa.sa_flags |= SA_RESTART;
    }
    sigfillset(&sa.sa_mask);
    assert(::sigaction(sig, &sa, nullptr) != -1);
  }

  static void ModifyFd(int epoll_fd, int fd, int ev) {
    epoll_event event;
    event.data.fd = fd;
    event.events = ev | EPOLLET | EPOLLONESHOT | EPOLLRDHUP;
    epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &event);
  }

  static void ShowError(int connfd, const char* info) {
    printf("%s\n", info);
    send(connfd, info, strlen(info), 0);
    close(connfd);
  }
}

#endif
