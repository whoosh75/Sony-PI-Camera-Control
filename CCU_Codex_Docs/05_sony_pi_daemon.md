# 05 — Sony Pi Daemon (multi-camera support)

## Purpose
Sony control runs on the Pi due to SDK/toolkit requirements. The Pi daemon abstracts Sony specifics and exposes CCU Bus semantics.

## Responsibilities
- Maintain multiple Sony camera sessions concurrently (one per static IP camera).
- Provide fast ACK responses and cached state snapshots to the Teensy.
- Ensure one camera cannot block others (per-camera worker or async model).
- Support slot mapping consistent with Teensy registry (static IP preferred).

## Network API
The Pi daemon listens for CCU Bus frames on UDP port 5555 (or configured).
- Receives CCU Bus SET_PARAM/ACTION/GET_STATE
- Executes SDK calls for the specified camera slot or IP
- Replies with ACK and optionally STATE_RESPONSE

## Camera identification
Preferred:
- use the slot IP from Teensy registry
Optional:
- discover cameras and expose LIST_CAMERAS for UI assignment

## Concurrency
Model:
- A `SonyCameraSession` object per camera.
- Each has:
  - connection state machine
  - command queue
  - poll loop (2–5 Hz) to update cached state
  - last error

## Rate limiting / coalescing
To handle encoder "scrubbing":
- coalesce SET_PARAM updates (keep only newest per param in the queue)
- enforce per-camera command rate caps

## Error handling
- timeouts -> error ACK, keep session alive
- disconnect -> mark offline and reconnect with backoff
- store last error string for UI display

## Suggested return codes
status_code:
- 0 OK
- 1 E_BAD_TARGET
- 2 E_NOT_CONNECTED
- 3 E_TIMEOUT
- 4 E_SDK_ERROR
- 5 E_UNSUPPORTED
