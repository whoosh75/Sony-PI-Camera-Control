# A74 Record Control Freeze (2026-02-08)

## Status
Frozen and validated on hardware.

Camera: `Sony ILCE-7M4 (A74)`
Transport: `USB`
Daemon: `pi_controller/src/main.cpp` + `pi_controller/src/sony_backend.cpp`
Protocol path: `CMD_RUNSTOP (0x10)` and `CMD_GET_STATUS (0x30)`

## Frozen Behavior
1. `CMD_RUNSTOP` payload `1` starts recording.
2. `CMD_RUNSTOP` payload `0` stops recording.
3. REC status is reported via `CMD_GET_STATUS` field `recording_state`.
4. CCU mapping is:
- `recording_state == 0` => `IDLE`
- `recording_state != 0` => `RECORDING`

## Frozen Command Sequence (A74)
In `SonyBackend::set_runstop(bool run)` for model `ILCE-7M4`:
1. Prepare camera for record command acceptance:
- `PriorityKeySettings = CrPriorityKey_PCRemote`
- `ExposureProgramMode = CrExposure_Movie_P`
2. Send `CrCommandId_MovieRecord` with:
- `CrCommandParam_Down` for start
- `CrCommandParam_Up` for stop
3. If MovieRecord is rejected (`CrError_Api_InvalidCalled` / `0x8402`), fallback to:
- `CrCommandId_MovieRecButtonToggle` (Down/Up)
- `CrCommandId_MovieRecButtonToggle2` (Down/Up)
- `CrCommandId_StreamButton` (Down/Up)

## Why This Is Frozen
Validated end-to-end with CCU serial monitor:
- Start transition: `rec_raw 0 -> 1`
- Stop transition: `rec_raw 1 -> 0`
- Status updates are fast and consistent during polling.

## Non-Regression Rule
Do not change A74 run/stop command order, parameters, or fallback list unless you re-run hardware validation and update this document.

## Required Validation If Modified
1. Build and restart daemon.
2. Press REC on CCU and verify:
- start succeeds
- stop succeeds
- `rec_raw` transitions `0 -> 1 -> 0`
3. Collect logs:
- CCU serial output
- `journalctl -u ccu-daemon -n 200 --no-pager`

## Related Files
- `pi_controller/src/sony_backend.cpp`
- `pi_controller/src/main.cpp`
- `docs/ccu_status_payload.md`
- `docs/TEENSY_CCU_CONTROLLER_BRIEF.md`
- `docs/USAGE_GUIDE.md`
