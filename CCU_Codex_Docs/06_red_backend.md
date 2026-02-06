# 06 — RED Backend Expectations (Teensy)

## Summary
RED control is executed directly on Teensy. RED supports UDP control and is a primary target.

## Requirements
- Implement a RED backend that can:
  - send commands derived from CCU Bus SET_PARAM/ACTION messages
  - query state and update the CCU registry cache
- Keep RED networking local (same LAN/Wi‑Fi as cameras); do not tunnel vendor UDP over RF.

## Notes
- RED RCP2 parameter documents (in project resources) should be used by Codex to map CCU param ids to RED parameter ids.
