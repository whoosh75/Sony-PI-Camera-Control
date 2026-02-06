# 07 — ARRI CAP Backend Expectations (Teensy)

## Summary
ARRI CAP is IP-based and should run on Teensy (no Pi requirement).

## Requirements
- Implement CAP client over Ethernet (UDP/TCP as required by CAP).
- Provide:
  - discovery (optional if static IP)
  - session establishment
  - parameter set/get
  - record start/stop
  - cached state updates for UI

## Static IP mode
Given cameras are mostly static IP:
- allow manual IP assignment per slot
- discovery is an optional convenience, not a dependency
