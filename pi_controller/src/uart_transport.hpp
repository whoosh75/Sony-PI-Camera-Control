#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace ccu {

class UartTransport {
public:
  bool open(const std::string& device, uint32_t baud);
  void close();

  // Non-blocking; returns full CCU1 frame length, or 0 if none.
  int recv_frame(uint8_t* out, size_t out_max);
  bool send_frame(const uint8_t* buf, size_t len);

private:
  int m_fd = -1;
  std::vector<uint8_t> m_buf;
  size_t m_rd = 0;

  void poll_rx();
  void compact();
};

} // namespace ccu
