cmake_minimum_required(VERSION 3.16)
project(ccu_daemon LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---- Sony CRSDK root (your existing SDK folder) ----
set(CRSDK_ROOT "" CACHE PATH "Path to Sony CRSDK bundle root")

add_executable(ccu_daemon
  src/main.cpp
  src/protocol.cpp
  src/udp_server.cpp
  src/sony_backend.cpp
  src/sony_sample/CrDebugString.cpp
  src/sony_sample/CameraDevice.cpp
  src/sony_sample/ConnectionInfo.cpp
  src/sony_sample/MessageDefine.cpp
  src/sony_sample/Text.cpp
)

add_executable(ccu_diag
  src/ccu_diag.cpp
)

target_compile_options(ccu_diag PRIVATE -fsigned-char)

# ---- CRSDK header compatibility (GCC 14) ----
# CRSDK uses -1 in enums with CrInt8 underlying type; GCC 14 can error if char is unsigned.
target_compile_options(ccu_daemon PRIVATE -fsigned-char)
target_link_libraries(ccu_daemon PRIVATE pthread dl)

# ---- Link Sony CRSDK (exact paths from your install) ----
if (CRSDK_ROOT)
  message(STATUS "Using CRSDK_ROOT=${CRSDK_ROOT}")

# ------------------------------------------------------------
# Runtime library search path (RPATH)
# Allows running without setting LD_LIBRARY_PATH.
# ------------------------------------------------------------
set(CRSDK_LIB_DIRS
  "${CRSDK_ROOT}/build"
  "${CRSDK_ROOT}/external/crsdk"
  "${CRSDK_ROOT}/external/crsdk/CrAdapter"
  "${CRSDK_ROOT}/external/opencv/Linux"
)

# Apply to all targets created after this point
set(CMAKE_BUILD_RPATH "${CRSDK_LIB_DIRS}")
set(CMAKE_INSTALL_RPATH "${CRSDK_LIB_DIRS}")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

target_include_directories(ccu_daemon PRIVATE
  ${CRSDK_ROOT}/app
  ${CRSDK_ROOT}/app/CRSDK
  ${CRSDK_ROOT}/external/opencv/include
  ${CMAKE_SOURCE_DIR}/src
)

  target_link_directories(ccu_daemon PRIVATE
    ${CRSDK_ROOT}/external/crsdk
    ${CRSDK_ROOT}/external/crsdk/CrAdapter
  )

  target_link_libraries(ccu_daemon PRIVATE
    Cr_Core
    monitor_protocol
    monitor_protocol_pf
    Cr_PTP_USB
    Cr_PTP_IP
    usb-1.0
    ssh2
  )

  set_target_properties(ccu_daemon PROPERTIES
    BUILD_RPATH "${CRSDK_ROOT}/external/crsdk;${CRSDK_ROOT}/external/crsdk/CrAdapter"
    INSTALL_RPATH "${CRSDK_ROOT}/external/crsdk;${CRSDK_ROOT}/external/crsdk/CrAdapter"
  )
  # ccu_diag: small diagnostic tool to list cameras and fingerprints
  target_include_directories(ccu_diag PRIVATE
    ${CRSDK_ROOT}/app
    ${CRSDK_ROOT}/app/CRSDK
    ${CMAKE_SOURCE_DIR}/src
  )

  target_link_directories(ccu_diag PRIVATE
    ${CRSDK_ROOT}/external/crsdk
    ${CRSDK_ROOT}/external/crsdk/CrAdapter
  )

  target_link_libraries(ccu_diag PRIVATE
    Cr_Core
    monitor_protocol
    monitor_protocol_pf
    Cr_PTP_USB
    Cr_PTP_IP
    usb-1.0
    ssh2
  )

  set_target_properties(ccu_diag PROPERTIES
    BUILD_RPATH "${CRSDK_ROOT}/external/crsdk;${CRSDK_ROOT}/external/crsdk/CrAdapter"
    INSTALL_RPATH "${CRSDK_ROOT}/external/crsdk;${CRSDK_ROOT}/external/crsdk/CrAdapter"
  )
endif()
add_executable(ccu_cli
  tools/ccu_cli.cpp
  src/protocol.cpp
)
