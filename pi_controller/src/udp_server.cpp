#include "udp_server.hpp"
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <cstring>

namespace ccu {

bool UdpServer::open(uint16_t port) {
  m_fd = ::socket(AF_INET, SOCK_DGRAM, 0);
  if (m_fd < 0) return false;

  int flags = ::fcntl(m_fd, F_GETFL, 0);
  ::fcntl(m_fd, F_SETFL, flags | O_NONBLOCK);

  int yes = 1;
  ::setsockopt(m_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(port);

  if (::bind(m_fd, (sockaddr*)&addr, sizeof(addr)) < 0) {
    ::close(m_fd);
    m_fd = -1;
    return false;
  }
  return true;
}

void UdpServer::close() {
  if (m_fd >= 0) ::close(m_fd);
  m_fd = -1;
}

int UdpServer::recv(uint8_t* buf, size_t max_len, sockaddr_in& from) {
  socklen_t sl = sizeof(from);
  const int n = (int)::recvfrom(m_fd, buf, max_len, 0, (sockaddr*)&from, &sl);
  if (n < 0) return 0;
  return n;
}

bool UdpServer::sendto(const uint8_t* buf, size_t len, const sockaddr_in& to) {
  const int n = (int)::sendto(m_fd, buf, len, 0, (const sockaddr*)&to, sizeof(to));
  return n == (int)len;
}

} // namespace ccu
