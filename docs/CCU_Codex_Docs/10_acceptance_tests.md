# 10 — Acceptance Tests (must pass)

## Transport + framing
1. UART framing resync:
   - inject random bytes + partial frames -> decoder recovers and accepts next valid frame.
2. CRC rejection:
   - corrupt one byte -> `rx_bad_crc` increments and frame is dropped.
3. AUTO selection:
   - with UDP active, UART also present: UDP chosen.
   - drop UDP (no frames for LINK_OK_MS) -> switch to UART.
   - restore UDP with hysteresis satisfied -> switch back.

## CCU Bus reliability
4. ACK_REQUIRED:
   - sender retries on timeout
   - receiver de-duplicates by (sender_id,msg_id)
   - command executes exactly once
5. GET_STATE:
   - returns cached state within one request/response cycle.

## Multi-Sony
6. Two Sony cameras, static IP:
   - both connect
   - REC_START on ALL triggers both within bounded time
   - one camera offline -> PARTIAL; online camera still works

## Mixed vendors
7. RED + Sony + ARRI present:
   - SET_PARAM on ALL routes to correct backend per slot
   - UI shows MIXED/UNIFIED states correctly

## Safety / UX
8. Barrier confirm rules (if implemented in UI layer):
   - mixed state record requires confirm
