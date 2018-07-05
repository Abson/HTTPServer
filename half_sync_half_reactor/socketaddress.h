#ifndef BASE_SOCKETADDRESS_H
#define BASE_SOCKETADDRESS_H 

#include "ipaddress.h"

class SocketAddress {
public:
  SocketAddress();

  SocketAddress(const std::string& hostname, int port);

  SocketAddress(uint32_t ip_as_host_order_interger, int port);

  SocketAddress(const IPAddress& ip, int port);

  SocketAddress(const SocketAddress& addr);
  const SocketAddress & operator=(const SocketAddress& addr);

  void SetIP(const std::string& hostname);

  void SetIP(const IPAddress& ip);

  void SetIP(uint32_t ip_as_host_order_interger);

  void SetPort(int port);

  const std::string hostname() { return hostname_; }

  uint32_t ip() const;

  const IPAddress& ipaddr() const;

  int family() const { return ip_.family(); }

  uint16_t port() const { return port_; }

  /*Return the scope id associated with this address. Scope IDs are a
   *necessary addition to IPV6 link-local address. with different network 
   *interface having different scope-ids for their link-local address.
   *IPv4 address do not have scope_ids and sockaddr_in structes do not have 
   *a field for them.*/
  int scope_id() const { return scope_id_; }
  void SetScopeID(int id) { scope_id_ = id; }

  void Clear();

  bool IPIsLoopbackIP() const;

  bool IsPrivateIP() const;

  void ToSockAddr(sockaddr_in* saddr) const;

  bool FromSockAddr(const sockaddr_in& saddr);

  std::string HostAsURIString() const;
  
  std::string PortAsString() const;

  std::string ToString() const;

  friend std::ostream& operator<<(std::ostream& os, const SocketAddress& addr);

  bool EqualIPs(const SocketAddress& addr) const;

  bool EqualPorts(const SocketAddress& addr) const;

  bool operator==(const SocketAddress& addr) const;

private:
  std::string hostname_;
  IPAddress ip_;
  uint16_t port_;
  int scope_id_;
  bool literal_;
};

SocketAddress EmptySocketAddressWithFamily(int family);

#endif
