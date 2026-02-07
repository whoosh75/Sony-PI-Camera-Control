# Camera Control Unit (CCU) — Codex Document Set
Version: 0.9 (draft for implementation)
Date: 2026-02-06 (Europe/London)

## Purpose
This document set is intended for an AI coding assistant (Codex) to implement the CCU project in a way that matches the user's established architecture and transport scheme from the motor-control project.

**System goal:** a unified multi-camera interface running on a Teensy 4.1 gateway that controls:
- **RED** cameras via UDP (and optionally Wi‑Fi/serial where supported)
- **ARRI** cameras via **ARRI CAP** over IP (implemented on Teensy)
- **Sony** cameras via a **Raspberry Pi 3B** daemon using Sony SDK/toolkit; Teensy talks to the Pi using the same internal CCU bus messages

**Networking assumption:** cameras are on **static IP** in nearly all cases.

## Core architectural rules (non-negotiable)
1. **Teensy owns the unified UI + multicam routing** (A–H + ALL).
2. **All camera vendors are controlled via a single internal "CCU Bus" intent protocol**.
3. **Transport is decoupled from payload**: the same CCU Bus frames run over UDP or transparent UART (RF modem).
4. **AUTO transport selection + fallback** must match the motor-control project's scheme:
   - link health measured by `age_ms` (time since last valid rx frame)
   - `link_ok` thresholds
   - deterministic “active transport” selection
5. **Sony camera control does not run on Teensy** (SDK requirement). Pi is Sony-only.

## What’s in this package
- `01_architecture.md` — overall topology and responsibilities
- `02_transports.md` — transport abstraction, AUTO selection, link health, reliability
- `03_ccu_bus_spec.md` — CCU Bus frame format, ACK/retry, message types
- `04_multicam_model.md` — camera registry, slot mapping, static IP strategy
- `05_sony_pi_daemon.md` — Pi service API + multi-camera management
- `06_red_backend.md` — RED backend expectations (UDP-centric)
- `07_arri_cap_backend.md` — ARRI CAP backend expectations (Teensy)
- `08_cst_v12_reference.md` — CST v1.2 reference (from motor project; included for parity + reuse)
- `09_repo_layout.md` — recommended repo/project layout (PlatformIO + Pi)
- `10_acceptance_tests.md` — concrete tests Codex must make pass

## Notes
- Some constants and exact byte layouts for CST v1.2 are included as **reference** and should be matched to your existing motor-control implementation. If Codex has access to your motor-control code, it must cross-check and align.
