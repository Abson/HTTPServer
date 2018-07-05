#include "socketaddress.h"
#include <iostream>
#include <sstream>

void SocketAddress::Clear() {
  hostname_.clear();
  literal_ = false;
  ip_ = IPAddress();
  port_ = 0;
  scope_id_ = 0;
}

SocketAddress::SocketAddress() {
  Clear();
}

SocketAddress::SocketAddress(const std::string& hostname, int port) {
  SetIP(hostname);
  SetPort(port);
}

SocketAddress::SocketAddress(uint32_t ip_as_host_order_interger, int port) {
  SetIP(IPAddress(ip_as_host_order_interger));
  SetPort(port);
}

SocketAddress::SocketAddress(const IPAddress& ip, int port) {
  SetIP(ip);
  SetPort(port);
}

SocketAddress::SocketAddress(const SocketAddress& addr) {
  this->operator=(addr);
}

const SocketAddress & SocketAddress::operator=(const SocketAddress& addr) {
  hostname_ = addr.hostname_;
  ip_ = addr.ip_;
  port_ = addr.port_;
  scope_id_ = addr.scope_id_;
  literal_ = addr.literal_;
  return *this;
}

void SocketAddress::SetIP(const std::string& hostname) {
  hostname_ = hostname;
  literal_ = IPFromString(hostname, &ip_);
  if (!literal_) {
    ip_ = IPAddress();
  }
  scope_id_ = 0;
}

void SocketAddress::SetIP(const IPAddress& ip) {
  hostname_.clear();
  literal_ = false;
  ip_  = ip;
  scope_id_ = 0;
}

void SocketAddress::SetIP(uint32_t ip_as_host_order_interger) {
  hostname_.clear();
  literal_ = false;
  ip_ = IPAddress(ip_as_host_order_interger);
  scope_id_ = 0;
}

void SocketAddress::SetPort(int port) {
  port_ = static_cast<uint16_t>(port);
}

bool SocketAddress::IPIsLoopbackIP() const {
  return IPIsLoopback(ip_) || (IPIsAny(ip_) && 
    0 == strcmp(hostname_.c_str(), "localhost"));
}

bool SocketAddress::IsPrivateIP() const {
  return IPIsPrivateNetWork(ip_);
}

void SocketAddress::ToSockAddr(sockaddr_in* saddr) const {

  memset(saddr, 0, sizeof(*saddr));

  if (ip_.family() != AF_INET) {
    saddr->sin_family = AF_UNSPEC;
    return;
  }

  saddr->sin_family = AF_INET;
  saddr->sin_port = htons(port_);
  if (IPIsAny(ip_)) {
    saddr->sin_addr.s_addr = INADDR_ANY;
  }
  else {
    saddr->sin_addr = ip_.ip4_address();
  }
}

bool SocketAddress::FromSockAddr(const sockaddr_in& saddr) {
  if (saddr.sin_family != AF_INET) 
    return false;

  SetIP(ntohl(saddr.sin_addr.s_addr));
  SetPort(ntohs(saddr.sin_port));
  literal_ = false;
  return true;
}

std::string SocketAddress::HostAsURIString() const {
  // If the hostname was a literal IP string, it may need to have square
  // brackets added (for SocketAddress::ToString()).
  if (!literal_ && !hostname_.empty()) {
    return hostname_;
  }
  if (ip_.family() == AF_INET6) {
    return "[" + ip_.ToString() + "]";
  }
  return ip_.ToString();
}

std::string SocketAddress::PortAsString() const {
  std::ostringstream ost;
  ost << port_;
  return ost.str();
}

uint32_t SocketAddress::ip() const {
  return ip_.v4AddressAsHostOrderInteger();
}

const IPAddress& SocketAddress::ipaddr() const {
  return ip_;
}

std::string SocketAddress::ToString() const {
  std::ostringstream ost;
  ost << *this;
  return ost.str();
}

std::ostream& operator<<(std::ostream& os, const SocketAddress& addr) {
  os << addr.HostAsURIString() << ":" << addr.port();
  return os;
}

bool SocketAddress::EqualIPs(const SocketAddress& addr) const {
  return (ip_ == addr.ip_  && ((!IPIsAny(ip_) && !IPIsUnspec(ip_)) || 
      hostname_ == addr.hostname_));
}

bool SocketAddress::EqualPorts(const SocketAddress& addr) const {
  return port_ == addr.port_;
}

bool SocketAddress::operator==(const SocketAddress& addr) const {
  return EqualIPs(addr) && EqualPorts(addr);
}

SocketAddress EmptySocketAddressWithFamily(int family) {
  if (family == AF_INET) {
    return SocketAddress(IPAddress(INADDR_ANY), 0);
  }
  else if (family == AF_INET6) {
    return SocketAddress(IPAddress(in6addr_any), 0);
  }
  return SocketAddress();
}
