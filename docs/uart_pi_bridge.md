# CCU ↔ Pi UART Transport (Transparent RF-UART Bridge)

This document describes how to implement UART transport on the Pi side for CCU1 frames. The UART link is **transparent** (CCU → RF → Pi), so the **exact CCU1 frame** used over UDP is carried over UART without modification.

> Scope: Sony-only on Pi. All non-Sony cameras remain on the Teensy CCU. The Pi daemon only replaces UDP receive/send with UART receive/send.

---

## 1) CCU1 Frame Format (byte-level)

CCU1 uses a fixed 16-byte header and CRC32 (IEEE reflected, poly 0xEDB88320). **Little-endian** fields.

```
Offset  Size  Field
0       4     MAGIC = 0x43 0x43 0x55 0x31  ('C''C''U''1')
4       1     version (1)
5       1     msg_type (0x01=request, 0x81=response)
6       2     payload_len (bytes)
8       4     seq
12      1     target_mask (bit0=A..bit7=H, 0xFF=ALL)
13      1     cmd_or_code
14      2     flags
16      N     payload
16+N    4     CRC32 over [0..(15+N)]
```

**Total frame length** = `16 + payload_len + 4`

CRC32 is the same as `crc32_ieee()` in [Imported Reference Docs/camera-control-full/pi_controller/src/protocol.cpp](Imported%20Reference%20Docs/camera-control-full/pi_controller/src/protocol.cpp).

---

## 2) UART Line Settings

- 8-N-1
- 115200 bps (or higher if your RF modem supports it)
- No flow control unless the modem requires it
- Raw binary (no escaping)

---

## 3) Pi UART Receiver (state machine)

UART is a byte stream; you must frame CCU1 packets yourself. Recommended approach:

1. **Ring buffer**: store incoming bytes.
2. **Scan for MAGIC**: `0x43 0x43 0x55 0x31`.
3. **Header parse**: when at least 16 bytes available, read header.
4. **Length check**: compute `frame_len = 16 + payload_len + 4`.
5. **Wait for full frame**: if buffer doesn’t contain full frame, wait for more bytes.
6. **CRC verify**: if CRC fails, discard one byte and resync (scan for MAGIC again).
7. **Dispatch**: pass the frame to existing `parse_packet()` and command handlers.

### Resync strategy
- On CRC fail, drop the first byte and rescan MAGIC.
- On invalid header (bad version or impossible payload_len), drop one byte and rescan.

### Payload limits
- Use the same payload limits as UDP (current buffer sizes are 512 bytes in the Pi daemon).

---

## 4) Pi UART Sender

- When you build a response frame (`build_resp_ack()`), write the full buffer to UART as a single write if possible.
- If the UART driver splits writes, that’s fine: the receiver is framed and CRC-protected.

---

## 5) Suggested Pi Implementation Steps

1. **Create UART transport** (new file `uart_transport.cpp/.hpp` or inline in daemon):
   - Open `/dev/ttyAMA0` or `/dev/serial0`.
   - Configure termios: raw mode, 115200, 8N1, no flow control.
   - Non-blocking `read()`.

2. **Receive loop**:
   - Replace `UdpServer::recv()` with a UART poll:
     - read bytes into ring buffer
     - emit one complete CCU1 frame at a time

3. **Send**:
   - Replace `UdpServer::sendto()` with `write(fd, buf, len)`.

4. **Reuse existing protocol**:
   - Keep `parse_packet()` and all command handlers as-is.
   - The frame contents are identical to UDP.

---

## 6) Example UART Decoder Pseudocode

```
while (true):
  read UART bytes into ring buffer
  while ring has >= 4:
    find MAGIC offset
    if not found: drop all but last 3 bytes
    if ring has < offset+16: break
    hdr = ring[offset..offset+15]
    payload_len = hdr.payload_len
    frame_len = 16 + payload_len + 4
    if ring has < offset+frame_len: break
    frame = ring[offset..offset+frame_len-1]
    if CRC ok:
       handle_frame(frame)
       drop bytes up to offset+frame_len
    else:
       drop 1 byte and rescan
```

---

## 7) Command/Response Behavior

Use **exactly the same** CCU1 commands and responses as UDP. The Teensy should not need to change any CCU1 payload logic.

- `CMD_DISCOVER (0x32)` for Sony discovery
- `CMD_GET_STATUS (0x30)` for battery/media status
- `CMD_GET_OPTIONS (0x20)` for menu options
- `CMD_SET_VALUE (0x40)` for menu value set
- `CMD_PARAM_STEP (0x41)` for encoder step

---

## 8) Operational Notes (RF link)

- Transparent RF modems often insert latency. Use CCU-side retry/backoff rather than making UART blocking.
- Keep frames small (under 256–512 bytes) to reduce RF collision.
- Ensure your RF modem does **not** add escaping or line endings.

---

## 9) Testing Checklist

- Loopback test with TX/RX directly connected, verify CCU1 CRCs.
- RF link test at minimum and maximum distances.
- Validate `CMD_GET_STATUS` cadence and CCU display refresh.
- Verify menu option lists populate over UART.

---

If you want, I can add the UART transport implementation in the Pi daemon in a follow-up.