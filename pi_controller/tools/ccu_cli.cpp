#include "../src/protocol.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

using namespace ccu;

static size_t build_req_runstop(uint8_t* out, size_t out_max, uint32_t seq, uint8_t target_mask, bool run) {
  const uint8_t payload[1] = {(uint8_t)(run ? 1 : 0)};
  const size_t payload_len = 1;
  const size_t total = sizeof(Header) + payload_len + 4;
  if (out_max < total) return 0;

  Header h{};
  h.magic = MAGIC;
  h.version = VER;
  h.msg_type = MSG_REQ_CMD;
  h.payload_len = (uint16_t)payload_len;
  h.seq = seq;
  h.target_mask = target_mask;
  h.cmd_or_code = CMD_RUNSTOP;
  h.flags = 0;

  std::memcpy(out, &h, sizeof(h));
  std::memcpy(out + sizeof(Header), payload, payload_len);

  const uint32_t crc = crc32_ieee(out, sizeof(Header) + payload_len);
  std::memcpy(out + sizeof(Header) + payload_len, &crc, 4);
  return total;
}

int main(int argc, char** argv) {
  if (argc < 4) {
    std::fprintf(stderr, "Usage: ccu_cli <ip> <port> <run|stop> [mask_hex]\n");
    return 2;
  }
  const char* ip = argv[1];
  const int port = std::atoi(argv[2]);
  const bool run = (std::string(argv[3]) == "run");
  const uint8_t mask = (argc >= 5) ? (uint8_t)std::strtoul(argv[4], nullptr, 16) : 0x01;

  int fd = ::socket(AF_INET, SOCK_DGRAM, 0);
  if (fd < 0) { std::perror("socket"); return 1; }

  sockaddr_in to{};
  to.sin_family = AF_INET;
  to.sin_port = htons((uint16_t)port);
  if (::inet_pton(AF_INET, ip, &to.sin_addr) != 1) {
    std::fprintf(stderr, "Bad IP\n");
    return 1;
  }

  uint8_t tx[256];
  uint8_t rx[256];

  const uint32_t seq = 1;
  const size_t n = build_req_runstop(tx, sizeof(tx), seq, mask, run);
  if (!n) return 1;

  if (::sendto(fd, tx, n, 0, (sockaddr*)&to, sizeof(to)) != (ssize_t)n) {
    std::perror("sendto");
    return 1;
  }

  timeval tv{0, 200000};
  setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

  sockaddr_in from{};
  socklen_t sl = sizeof(from);
  const int rn = (int)::recvfrom(fd, rx, sizeof(rx), 0, (sockaddr*)&from, &sl);
  if (rn <= 0) { std::fprintf(stderr, "No response\n"); return 1; }

  Header h{};
  const uint8_t* pl = nullptr;
  size_t pl_len = 0;
  uint8_t err = 0;

  if (!parse_packet(rx, (size_t)rn, h, pl, pl_len, err)) {
    std::fprintf(stderr, "Bad ACK\n");
    return 1;
  }

  std::printf("ACK seq=%u code=%u target=0x%02X\n", h.seq, h.cmd_or_code, h.target_mask);
  if (pl_len >= 5) {
    std::printf(" ok=0x%02X fail=0x%02X busy=0x%02X run=0x%02X known=0x%02X\n",
      pl[0], pl[1], pl[2], pl[3], pl[4]);
  }

  ::close(fd);
  return 0;
}
