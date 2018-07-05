#ifndef BASE_IPADDRESS_H
#define BASE_IPADDRESS_H

#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>

class IPAddress {
public:
  IPAddress() : family_(AF_UNSPEC) {
    memset(&u_, '\0', sizeof(u_));
  }

  explicit IPAddress(const in_addr& ip4) : family_(AF_INET) {
    memset(&u_, '\0', sizeof(u_));
    u_.ip4 = ip4;
  }

  explicit IPAddress(const in6_addr& ip6) : family_(AF_INET6) {
    memset(&u_, '\0', sizeof(u_));
    u_.ip6 = ip6;
  }

  explicit IPAddress(uint32_t ip_in_host_byte_order) : family_(AF_INET) {
    memset(&u_, '\0', sizeof(u_));
    u_.ip4.s_addr = htonl(ip_in_host_byte_order);
  }

  IPAddress(const IPAddress& other) : family_(other.family_) {
    memcpy(&u_, &other.u_, sizeof(u_));
  }
  
  const IPAddress & operator=(const IPAddress& other) {
    family_ = other.family_;
    memcpy(&u_, &other.u_, sizeof(u_));
    return *this;
  }

  int family() const { return family_; }

  in_addr ip4_address() const { return u_.ip4; }

  in6_addr ip6_address() const { return u_.ip6; }

  size_t Size() const;

  std::string ToString() const;

  std::string ToSensitiveString() const;

  bool IsNil() const;

  uint32_t v4AddressAsHostOrderInteger() const;

  bool operator==(const IPAddress& other) const;

  /*将一个ipv4地址转换成一个ipv6地址*/
  IPAddress AsIPV6Address() const;

  virtual ~IPAddress() {}

private:
  int family_;
  union {
    in_addr ip4;
    in6_addr ip6;
  } u_;
};

bool IPFromString(const std::string& str, IPAddress* out);
bool IPIsLoopback(const IPAddress& ip);
bool IPIsAny(const IPAddress& ip);
bool IPIsPrivateNetWork(const IPAddress& ip);
bool IPIsUnspec(const IPAddress& ip);

#endif
