# 09 — Repo Layout (PlatformIO + Pi)

## Recommended structure
```
/camera-control/
  /teensy/
    platformio.ini
    /src/
      main.cpp
    /lib/
      /ccu_bus/
        ccu_bus_frame.h/.cpp
        ccu_transport_iface.h
        ccu_transport_udp.h/.cpp
        ccu_transport_uart.h/.cpp
        ccu_transport_auto.h/.cpp
        ccu_router.h/.cpp
        ccu_registry.h/.cpp
      /backend_red/
      /backend_arri_cap/
      /backend_sony_proxy/
  /pi/
    /sony_daemon/
      CMakeLists.txt
      src/
        main.cpp
        net_ccu_endpoint.cpp
        sony_sessions.cpp
        sony_state_cache.cpp
```

## Build tools
- Teensy: PlatformIO + Teensyduino core
- Pi: CMake (recommended) or PlatformIO (optional)
- Remote SSH used for building/running daemon and logs.

## Coding rules
- CCU Bus framing shared across Teensy and Pi (keep identical).
- Transport implementations do not interpret payload.
- Backends interpret payload and return ACK/state updates.
