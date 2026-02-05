#pragma once
#include <cstdint>
#include <cstddef>
#include <netinet/in.h>

namespace ccu {

class UdpServer {
public:
  bool open(uint16_t port);
  void close();

  // Non-blocking recv; returns bytes, 0 if none.
  int recv(uint8_t* buf, size_t max_len, sockaddr_in& from);
  bool sendto(const uint8_t* buf, size_t len, const sockaddr_in& to);

private:
  int m_fd = -1;
};

} // namespace ccu
