#include "protocol.hpp"
#include <cstring>

namespace ccu {

uint32_t crc32_ieee(const uint8_t* data, size_t len) {
  uint32_t crc = 0xFFFFFFFFu;
  for (size_t i = 0; i < len; i++) {
    crc ^= data[i];
    for (int b = 0; b < 8; b++) {
      uint32_t mask = (uint32_t)-(int)(crc & 1u);
      crc = (crc >> 1) ^ (0xEDB88320u & mask);
    }
  }
  return ~crc;
}

static inline uint32_t rd32(const uint8_t* p) {
  uint32_t v;
  std::memcpy(&v, p, 4);
  return v;
}

bool parse_packet(const uint8_t* buf, size_t len, Header& out_h,
                  const uint8_t*& out_payload, size_t& out_pl_len,
                  uint8_t& out_err) {
  out_err = RESP_OK;
  if (len < sizeof(Header) + 4) { out_err = RESP_BAD_FORMAT; return false; }

  std::memcpy(&out_h, buf, sizeof(Header));
  if (out_h.magic != MAGIC || out_h.version != VER) { out_err = RESP_BAD_FORMAT; return false; }

  const size_t payload_len = (size_t)out_h.payload_len;
  if (sizeof(Header) + payload_len + 4 != len) { out_err = RESP_BAD_FORMAT; return false; }

  const uint32_t got_crc = rd32(buf + sizeof(Header) + payload_len);
  const uint32_t calc = crc32_ieee(buf, sizeof(Header) + payload_len);
  if (got_crc != calc) { out_err = RESP_BAD_CRC; return false; }

  out_payload = buf + sizeof(Header);
  out_pl_len = payload_len;
  return true;
}

size_t build_resp_ack(uint8_t* out, size_t out_max,
                      uint32_t seq, uint8_t target_mask, uint8_t resp_code,
                      const uint8_t* payload, size_t payload_len) {
  const size_t total = sizeof(Header) + payload_len + 4;
  if (out_max < total) return 0;

  Header h{};
  h.magic = MAGIC;
  h.version = VER;
  h.msg_type = MSG_RESP_ACK;
  h.payload_len = (uint16_t)payload_len;
  h.seq = seq;
  h.target_mask = target_mask;
  h.cmd_or_code = resp_code;
  h.flags = 0;

  std::memcpy(out, &h, sizeof(h));
  if (payload_len && payload) std::memcpy(out + sizeof(Header), payload, payload_len);

  const uint32_t crc = crc32_ieee(out, sizeof(Header) + payload_len);
  std::memcpy(out + sizeof(Header) + payload_len, &crc, 4);
  return total;
}

} // namespace ccu

