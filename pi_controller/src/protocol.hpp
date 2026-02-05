#pragma once
#include <cstdint>
#include <cstddef>


namespace ccu {

static constexpr uint32_t MAGIC = 0x43435531u; // 'CCU1'
static constexpr uint8_t  VER   = 1;

enum : uint8_t {
  MSG_REQ_CMD  = 0x01,
  MSG_RESP_ACK = 0x81,
};

enum : uint8_t {
  CMD_RUNSTOP = 0x10,
};

enum : uint8_t {
  RESP_OK         = 0x00,
  RESP_BAD_CRC    = 0x01,
  RESP_BAD_FORMAT = 0x02,
  RESP_UNKNOWN    = 0x03,
};

#pragma pack(push, 1)
struct Header {
  uint32_t magic;
  uint8_t  version;
  uint8_t  msg_type;
  uint16_t payload_len;
  uint32_t seq;
  uint8_t  target_mask;     // bit0=A..bit7=H, 0xFF=ALL
  uint8_t  cmd_or_code;     // cmd for REQ, resp_code for ACK
  uint16_t flags;
};
#pragma pack(pop)

static_assert(sizeof(Header) == 16, "Header must be 16 bytes");

uint32_t crc32_ieee(const uint8_t* data, size_t len);

bool parse_packet(const uint8_t* buf, size_t len, Header& out_h,
                  const uint8_t*& out_payload, size_t& out_pl_len,
                  uint8_t& out_err);

size_t build_resp_ack(uint8_t* out, size_t out_max,
                      uint32_t seq, uint8_t target_mask, uint8_t resp_code,
                      const uint8_t* payload, size_t payload_len);

} // namespace ccu
