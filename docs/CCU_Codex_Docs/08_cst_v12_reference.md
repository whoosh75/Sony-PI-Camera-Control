# 08 — CST v1.2 Reference (Motor Control Project Parity)

## Purpose
The CCU project should reuse the proven transport patterns from the motor-control project. CST v1.2 is included here as a reference for:
- framing discipline
- CRC32C usage
- stable parsing and resync strategies
- link health metrics (age_ms, ok/bad_crc/bad_fmt)

## Known CST v1.2 elements (from existing RX decode snippet)
- Payload ends with CRC32C (little-endian).
- Starts with `CST_MAGIC` 4 bytes.
- Version bytes at offsets 4 and 5: major=1 minor=2.
- Contains `header_len` at offset 6 (u16 LE).
- The decoder verifies:
  - len >= 28
  - CRC32C matches
  - magic and version match
- The design supports sparse channel updates (TLV-like) and per-axis mode bytes.

## Important stability requirement (Teensy Serial2)
In motor project you required:
- `SERIAL2_TX_BUFFER_SIZE 512`
- `SERIAL2_RX_BUFFER_SIZE 256`
defined before Arduino/HardwareSerial headers.

Carry the same discipline to any UART used for CCU Bus.

## Takeaway for CCU project
- CCU Bus uses a similar CRC32C-at-end framing.
- Do not print in hot paths; throttle to ~1 Hz for diagnostics.
- Feed parsers from a byte-drain loop, compute gates once per Arduino loop (pattern from RX).
