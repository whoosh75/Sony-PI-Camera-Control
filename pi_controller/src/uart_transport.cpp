#include "uart_transport.hpp"
#include "protocol.hpp"
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <cstring>
#include <algorithm>

namespace ccu {

static speed_t baud_to_speed(uint32_t baud) {
  switch (baud) {
    case 9600: return B9600;
    case 19200: return B19200;
    case 38400: return B38400;
    case 57600: return B57600;
    case 115200: return B115200;
    case 230400: return B230400;
    case 460800: return B460800;
    case 921600: return B921600;
    default: return B115200;
  }
}

bool UartTransport::open(const std::string& device, uint32_t baud) {
  close();
  m_fd = ::open(device.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
  if (m_fd < 0) return false;

  termios tio{};
  if (::tcgetattr(m_fd, &tio) != 0) {
    close();
    return false;
  }

  ::cfmakeraw(&tio);
  tio.c_cflag |= (CLOCAL | CREAD);
  tio.c_cflag &= ~CRTSCTS; // no HW flow control
  tio.c_cflag &= ~PARENB;
  tio.c_cflag &= ~CSTOPB;
  tio.c_cflag &= ~CSIZE;
  tio.c_cflag |= CS8;

  const speed_t spd = baud_to_speed(baud);
  ::cfsetispeed(&tio, spd);
  ::cfsetospeed(&tio, spd);

  if (::tcsetattr(m_fd, TCSANOW, &tio) != 0) {
    close();
    return false;
  }

  m_buf.clear();
  m_rd = 0;
  return true;
}

void UartTransport::close() {
  if (m_fd >= 0) ::close(m_fd);
  m_fd = -1;
  m_buf.clear();
  m_rd = 0;
}

void UartTransport::poll_rx() {
  if (m_fd < 0) return;
  uint8_t tmp[256];
  while (true) {
    const int n = (int)::read(m_fd, tmp, sizeof(tmp));
    if (n <= 0) break;
    m_buf.insert(m_buf.end(), tmp, tmp + n);
  }
}

void UartTransport::compact() {
  if (m_rd == 0) return;
  if (m_rd >= m_buf.size()) {
    m_buf.clear();
    m_rd = 0;
    return;
  }
  if (m_rd > 1024 || m_rd > (m_buf.size() / 2)) {
    m_buf.erase(m_buf.begin(), m_buf.begin() + (long)m_rd);
    m_rd = 0;
  }
}

static inline uint32_t rd32_le(const uint8_t* p) {
  return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

int UartTransport::recv_frame(uint8_t* out, size_t out_max) {
  poll_rx();

  const uint8_t magic[4] = { 'C', 'C', 'U', '1' };

  while (true) {
    if (m_buf.size() - m_rd < 4) { compact(); return 0; }

    size_t pos = m_rd;
    bool found = false;
    for (; pos + 4 <= m_buf.size(); ++pos) {
      if (std::memcmp(m_buf.data() + pos, magic, 4) == 0) {
        found = true;
        break;
      }
    }

    if (!found) {
      // Keep last 3 bytes in case magic spans reads.
      if (m_buf.size() - m_rd > 3) {
        m_rd = m_buf.size() - 3;
      }
      compact();
      return 0;
    }

    if (m_buf.size() - pos < sizeof(Header)) {
      m_rd = pos;
      compact();
      return 0;
    }

    const uint8_t* hdr = m_buf.data() + pos;
    const uint16_t payload_len = (uint16_t)hdr[6] | ((uint16_t)hdr[7] << 8);
    const size_t frame_len = sizeof(Header) + (size_t)payload_len + 4;

    if (frame_len > out_max || frame_len > 2048) {
      // Bad length; skip one byte and resync.
      m_rd = pos + 1;
      continue;
    }

    if (m_buf.size() - pos < frame_len) {
      m_rd = pos;
      compact();
      return 0;
    }

    const uint8_t* frame = m_buf.data() + pos;
    const uint32_t got_crc = rd32_le(frame + sizeof(Header) + payload_len);
    const uint32_t calc_crc = crc32_ieee(frame, sizeof(Header) + payload_len);
    if (got_crc != calc_crc) {
      m_rd = pos + 1;
      continue;
    }

    std::memcpy(out, frame, frame_len);
    m_rd = pos + frame_len;
    compact();
    return (int)frame_len;
  }
}

bool UartTransport::send_frame(const uint8_t* buf, size_t len) {
  if (m_fd < 0) return false;
  const ssize_t n = ::write(m_fd, buf, len);
  return n == (ssize_t)len;
}

} // namespace ccu
