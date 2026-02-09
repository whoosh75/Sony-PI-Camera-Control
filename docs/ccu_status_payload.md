# CCU1 CMD_GET_STATUS payload update (Recording State)

Date: 2026-02-08

## Summary
The Pi daemon now appends **`recording_state`** to the `CMD_GET_STATUS (0x30)` response payload. This allows the CCU to refresh REC state from status polling instead of relying on stale UI state.

## Payload Layout (LE)
All integer fields are **uint32 little‑endian** unless noted.

**Previous payload** (44 bytes + tail):
1. `battery_level`
2. `battery_remain`
3. `battery_remain_unit`
4. `recording_media`
5. `movie_recording_media`
6. `media_slot1_status`
7. `media_slot1_remaining_number`
8. `media_slot1_remaining_time`
9. `media_slot2_status`
10. `media_slot2_remaining_number`
11. `media_slot2_remaining_time`
12. `conn_type` (uint8)
13. `model_len` (uint8)
14. `model` (bytes, length = `model_len`, max 32)

**New payload** (48 bytes + tail):
1–11. **same as above**
12. `recording_state` (**NEW**)  
13. `conn_type` (uint8)  
14. `model_len` (uint8)  
15. `model` (bytes)

## Recording State
`recording_state` is the raw CRSDK `CrDeviceProperty_RecordingState` value. The CCU should treat:
- `0` as **not recording**
- any non‑zero value as **recording** (unless you choose to map specific vendor codes)

For A74 USB freeze baseline, `recording_state` is now validated in CCU serial as:
- `0` => `IDLE`
- `1` => `RECORDING`

Reference freeze doc: `docs/A74_RECORD_FREEZE_2026-02-08.md`

## Required CCU Changes
1. **Status parsing**: insert a 4‑byte `recording_state` field **after** `media_slot2_remaining_time`.
2. **UI update**: set REC indicator based on `recording_state` from the status poll.
3. **Backward compatibility** (optional): if payload length is 44 bytes, assume `recording_state = unknown` and keep current behavior.

## Rationale
CCU currently polls status; without a recording state field, the REC display can become stale between commands. Adding this field lets CCU refresh REC state with each `CMD_GET_STATUS` response.

## Code References (Pi)
- Payload packing: [pi_controller/src/main.cpp](pi_controller/src/main.cpp)
- Status source: [pi_controller/src/sony_backend.cpp](pi_controller/src/sony_backend.cpp)
- Status struct: [pi_controller/src/sony_backend.hpp](pi_controller/src/sony_backend.hpp)
