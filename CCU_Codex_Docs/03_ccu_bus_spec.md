# 03 — CCU Bus Protocol Specification (transport-agnostic)

## Overview
CCU Bus is the internal command protocol between:
- UI host and Teensy gateway (optional remote UI in future)
- Teensy gateway and Pi Sony daemon
- Any other future adapters

CCU Bus runs identically over UDP or UART.

## Frame format (byte-level)
All multi-byte integer fields are **little-endian** unless stated.

```
Offset  Size  Field
0       4     MAGIC = 0x43 0x43 0x55 0x42   ('C''C''U''B')
4       1     ver_major (1)
5       1     ver_minor (0)
6       2     header_len (bytes, includes fixed header + optional header TLVs)
8       2     payload_len (bytes)
10      2     flags
12      2     sender_id      (0..65535)
14      2     msg_id         (wraps)
16      2     msg_type       (see below)
18      2     target         (A=0..H=7, ALL=0xFFFF)
20      4     timestamp_ms   (sender local time)
24      N     [optional header TLVs]  (header_len - 24)
24+N    payload_len  payload bytes
...     4     CRC32C over bytes [0 .. (24+N+payload_len-1)]
```

### Flags
- bit0: `ACK_REQUIRED`
- bit1: `IS_ACK` (this frame is an ACK response)
- bit2: `IS_ERROR` (ACK indicates failure; payload includes error)
- bit3: `IS_TELEMETRY` (not stored; may be dropped)
- remaining: reserved

### CRC
CRC32C (Castagnoli) of all bytes excluding the CRC field, appended little-endian.

## Message types
`msg_type` defines how to parse payload.

Core set (MVP):
- 0x0001 HELLO
- 0x0002 CAPS_REQUEST
- 0x0003 CAPS_RESPONSE
- 0x0010 GET_STATE
- 0x0011 STATE_RESPONSE
- 0x0020 SET_PARAM_U32
- 0x0021 SET_PARAM_I32
- 0x0022 SET_PARAM_F32
- 0x0023 SET_PARAM_ENUM
- 0x0030 ACTION (record start/stop, focus push, etc.)
- 0x00F0 HEARTBEAT

## Parameter model
Parameters are keyed by:
- `param_group` (vendor-neutral group: EXPOSURE, WB, LENS, MEDIA, TIMEcode, etc.)
- `param_id` (specific parameter within group)

Suggested payload for SET_PARAM_*:
```
uint16 param_group
uint16 param_id
<value>  (type depends on msg_type)
```

## ACTION model
Payload:
```
uint16 action_id
uint16 action_arg0
uint32 action_arg1
```

Examples:
- `action_id=REC_START` arg0 unused
- `action_id=REC_STOP`
- `action_id=FOCUS_PUSH_AF`
- `action_id=TOGGLE_TALLY`

## ACK payload
If `IS_ACK` set:
Payload:
```
uint16 acked_msg_type
uint16 acked_msg_id
uint16 status_code  (0=OK, nonzero=error)
uint16 detail_code  (optional)
```

Receiver must send ACK when `ACK_REQUIRED` is set.

## De-duplication
Receiver must ignore duplicate commands by remembering last N `(sender_id,msg_id)` pairs, per sender.

## UDP mapping
- Default port: `CCU_UDP_PORT = 5555` (adjust as needed)
- For request/response, reply to sender's `(ip,port)`.

## UART mapping
- Use the same exact frames.
- A UART decoder reads bytes, finds MAGIC, then uses header_len/payload_len to know frame length, validates CRC, then emits frame.
- On CRC fail, resync by scanning for MAGIC.
