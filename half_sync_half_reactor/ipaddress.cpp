#include "ipaddress.h"
#include "commont/stringutils.h"

#define NDEBUG 1

static const in6_addr kV4MappedPrefix = {{{0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                           0xFF, 0xFF, 0}}};

static const in6_addr kPrivateNetworkPrefix = {{{0xFD}}};

static bool IPIsHelper(const IPAddress& ip, const in6_addr& tomatch, int length);

uint32_t IPAddress::v4AddressAsHostOrderInteger() const {
  if (family_ == AF_INET) {
    return ntohl(u_.ip4.s_addr);
  }
  return 0;
}

bool IPAddress::IsNil() const {
  return family_ == AF_UNSPEC;
}

size_t IPAddress::Size() const {
  switch (family_) {
    case AF_INET:
      return sizeof(in_addr);
    case AF_INET6:
      return sizeof(in6_addr);
  }
  return 0;
}

std::string IPAddress::ToString() const {
  if (family_ != AF_INET && family_ != AF_INET6) {
    return std::string();
  }
  char buf[INET6_ADDRSTRLEN] = {0};
  const void* src = &u_.ip4;
  if (family_ == AF_INET6) {
    src = &u_.ip6;
  }
  if (inet_ntop(family_, src, buf, sizeof(buf))) {
    return std::string();
  }
  return std::string(buf);
}

std::string IPAddress::ToSensitiveString() const {
#if !defined (NDEBUG)
  return ToString();
#else
  switch (family_) {
    case AF_INET:
      {
        std::string address = ToString();
        size_t find_pos = address.rfind('.');
        if (find_pos == std::string::npos) {
          return std::string();
        }
        address.resize(find_pos);
        address += ".x";
        return address;
      }
    case AF_INET6:
      {
        std::string result;
        result.resize(INET6_ADDRSTRLEN);
        in6_addr addr = ip6_address();
        size_t len = sprintfn(&result[0], result.size(), "%x:%x:%x:x:x:x:x:x", 
            (addr.s6_addr[0] >> 8) + addr.s6_addr[1],
            (addr.s6_addr[2] << 8) + addr.s6_addr[3], 
            (addr.s6_addr[4] << 8) + addr.s6_addr[5]);
        result.resize(len);
        return result;
      }
  }
  return std::string();
#endif
}

bool IPAddress::operator==(const IPAddress& other) const {
  if (family_ != other.family_) {
    return false;
  }
  if (family_ == AF_INET) {
    return memcmp(&u_.ip4, &other.u_.ip4, sizeof(u_.ip4)) == 0;
  }
  if (family_ == AF_INET6) {
    return memcmp(&u_.ip6, &other.u_.ip6, sizeof(u_.ip6)) == 0;
  }
  return family_ == AF_UNSPEC;
}

bool IPFromString(const std::string& str, IPAddress* out) {
  if (!out) {
    return false;
  }
  in_addr addr;
  if (inet_pton(AF_INET, str.c_str(), &addr) == 0) {
    in6_addr addr6;
    if (inet_pton(AF_INET6, str.c_str(), &addr6) == 0) {
      *out = IPAddress();
      return false;
    }
    *out = IPAddress(addr6);
    return true; 
  }
  *out = IPAddress(addr);
  return true;
}

IPAddress IPAddress::AsIPV6Address() const {
  /*如果已经是ipv6的话，就没必要转了*/
  if (AF_INET != family_) {
    return *this;
  }
  in6_addr addr6 = kV4MappedPrefix;
  ::memcpy(&addr6.s6_addr[12], &u_.ip4.s_addr, sizeof(u_.ip4.s_addr));
  return IPAddress(addr6);
}

bool IPIsAny(const IPAddress& ip) {
  switch (ip.family()) {
    case AF_INET:
      {
        return ip == IPAddress(INADDR_ANY);
      }
    case AF_INET6:
      {
        return ip == IPAddress(in6addr_any) || ip == IPAddress(kV4MappedPrefix);
      }
    case AF_UNSPEC:
      {
        return false;
      }
  }
  return false;
}

static bool IPIsLoopbackV4(const IPAddress& ip) {
  uint32_t ip_in_host_order = ip.v4AddressAsHostOrderInteger();
  return ((ip_in_host_order >> 24) == 127);
}

static bool IPIsLoopbackV6(const IPAddress& ip) {
  return ip == IPAddress(in6addr_loopback);
}

bool IPIsLoopback(const IPAddress& ip) {
  switch (ip.family()) {
    case AF_INET:
      {
        return IPIsLoopbackV4(ip);
      }
    case AF_INET6: 
      {
        return IPIsLoopbackV6(ip);
      }
  }
  return false;
}

bool IPIsHelper(const IPAddress& ip, const in6_addr& tomatch, int length) {
  // Helper method for checking IP prefix matches (but only on whole byte
  // lengths). Length is in bits.
  in6_addr addr = ip.ip6_address();
  /*memcmp 的length用的是字节，但是外部传进来是bits，所以要左移动三位*/
  return ::memcmp(&addr, &tomatch, (length >> 3)) == 0;
}

/*局域网ip地址为 10.x.x.x 或 172.1.x.x 或 192.168.x.x*/
static bool IPIsPrivateNetWorkV4(const IPAddress& ip) {
  uint32_t ip_in_host_order = ip.v4AddressAsHostOrderInteger();
  return ((ip_in_host_order >> 24 == 10) || 
      (ip_in_host_order >> 20) == ((172 << 4) | 1 ) || 
      (ip_in_host_order >> 16) == ((192 << 8) | 168));
}

static bool IPIsPrivateNetWorkV6(const IPAddress& ip) {
  return IPIsHelper(ip, kPrivateNetworkPrefix, 8);
}

bool IPIsPrivateNetWork(const IPAddress& ip) {
  switch (ip.family()) {
    case AF_INET:
      {
        return IPIsPrivateNetWorkV4(ip);
      }
    case AF_INET6:
      {
        return IPIsPrivateNetWorkV6(ip);
      }
  }
  return false;
}

bool IPIsUnspec(const IPAddress& ip) {
  return ip.family() == AF_UNSPEC;
}
