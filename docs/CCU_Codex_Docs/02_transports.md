# 02 — Transports (AUTO selection, fallback, link health)

## Goals
- Run the same CCU Bus frames over:
  - UDP (Ethernet/Wi‑Fi LAN)
  - transparent UART (e.g., 868MHz RF modem)
- Provide a motor-project-like transport selection:
  - One "active transport" at a time (unless explicitly mirroring)
  - `age_ms` tracking, `link_ok`, error counters
  - AUTO mode chooses best available link

## Transport interface (Teensy)
Implement an abstract interface `ICcuTransport` with:
- `begin()`
- `poll_rx(now_ms)`  // drain bytes and feed the CCU frame decoder
- `send_frame(ptr,len)`
- `age_ms(now_ms)`
- `link_ok(now_ms)`
- `stats()` {rx_ok, rx_bad_crc, rx_bad_fmt, last_rx_ms}

## Link health and thresholds
Suggested defaults (match motor project if different):
- `LINK_OK_MS = 250`   // link considered OK if last_rx within 250ms
- `LINK_DROP_MS = 500` // consider hard drop beyond this (hysteresis)

Expose these as constants/config.

## AUTO selection behavior
Priority order (recommended):
1. UDP (LAN/Wi‑Fi)
2. UART (RF modem)

Selection:
- If active transport `link_ok`: keep it.
- If active transport not OK:
  - pick highest priority transport with `link_ok`.
- Optional hysteresis:
  - require N consecutive valid frames before switching back to preferred link.

## Reliability model
CCU Bus supports two reliability modes per message:
- `ACK_REQUIRED`: sender retries if no ACK within timeout
- `NO_ACK`: fire-and-forget for telemetry or non-critical updates

Recommended:
- Commands that change camera state: ACK_REQUIRED
- Periodic state/telemetry: NO_ACK or ACK optional

Retry policy (suggested):
- `ACK_TIMEOUT_MS = 120`
- `MAX_RETRIES = 3`
- receiver must de-duplicate by `(sender_id,msg_id)` to avoid double-actions (e.g., double record)

## UART specifics (transparent modem)
UART is a byte stream: must use framing:
- magic SOF + length + CRC
- decoder must resync after errors by scanning for SOF.

Avoid large prints in hot paths (same as motor project stability lessons).
