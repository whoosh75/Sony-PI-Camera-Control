# 04 — Multi-camera Model (slots, static IP, routing)

## Slots and targets
- Slots A..H mapped to indices 0..7
- Special target ALL = 0xFFFF
- Teensy UI always expresses actions against a target; router expands ALL into per-slot operations.

## Camera registry entry
Each slot has:
- `vendor` enum: RED, ARRI, SONY, UNKNOWN
- `ip` (static)
- `port` (vendor default or configured)
- `online` boolean
- `last_state` cached snapshot
- `caps` capability flags (per vendor/model)

Persist registry on Teensy (flash) and optionally on Pi for Sony slot mapping.

## Static IP strategy (Sony)
Given static IP in most cases:
- Slot stores Sony camera IP.
- Pi uses that IP as the primary connection target.
- If IP changes (rare), Pi can still optionally discover and suggest updates.

## Routing logic
Given a CCU Bus command (from UI or remote control plane):
1. Determine target slot(s).
2. For each slot:
   - if vendor == SONY: forward to Pi Sony daemon (CCU-over-UDP)
   - if vendor == RED: send via RED backend directly
   - if vendor == ARRI: send via CAP backend directly
3. Collect ACKs; UI shows unified/mixed/partial states per spec.

## UI state aggregation
For group operations (ALL):
- Determine group state:
  - UNIFIED: all in same state
  - MIXED: multiple states present
  - PARTIAL: some offline/unavailable
- Some actions require barrier confirm if state is MIXED/PARTIAL (per your UI rules)
