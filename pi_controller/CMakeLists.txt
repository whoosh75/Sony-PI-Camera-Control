cmake_minimum_required(VERSION 3.16)
project(ccu_daemon LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Allow includes like "shared/ccu-interface/ccu_link_protocol_v1.h"
# when building from the pi_controller subproject.
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)

# ---- Sony CRSDK root (your existing SDK folder) ----
set(CRSDK_ROOT "" CACHE PATH "Path to Sony CRSDK bundle root")

# Create a shared library with Sony sample code
add_library(sony_sample STATIC
  src/sony_sample/CrDebugString.cpp
  src/sony_sample/CameraDevice.cpp
  src/sony_sample/ConnectionInfo.cpp
  src/sony_sample/MessageDefine.cpp
  src/sony_sample/Text.cpp
)

add_executable(ccu_daemon
  src/main.cpp
  src/protocol.cpp
  src/udp_server.cpp
  src/uart_transport.cpp
  src/sony_backend.cpp
)

add_executable(ccu_diag
  src/ccu_diag.cpp
)

add_executable(sony_a74_usb_test
  src/sony_a74_usb_test.cpp
)

add_executable(movie_record_test
  src/movie_record_test.cpp
)

add_executable(a74_usb_record_test
  src/a74_usb_record_test.cpp
)

add_executable(a74_usb_settings_test
  src/a74_usb_settings_test.cpp
)

add_executable(a74_usb_stills_test
  src/a74_usb_stills_test.cpp
)

add_executable(a74_eth_settings_test
  src/a74_eth_settings_test.cpp
)

add_executable(a74_eth_stills_test
  src/a74_eth_stills_test.cpp
)

add_executable(minimal_test
  src/minimal_test.cpp
)

add_executable(simple_sdk_test
  src/simple_sdk_test.cpp
)

add_executable(simple_recording_test
  src/simple_recording_test.cpp
)

add_executable(direct_sony_api
  src/direct_sony_api.cpp
)

add_executable(remotecli_test
  src/remotecli_test.cpp
)

add_executable(working_sdk_test
  src/working_sdk_test.cpp
  src/OpenCVWrapper.cpp
)

add_executable(sony_a74_direct_api
  src/sony_a74_direct_api.cpp
  src/OpenCVWrapper.cpp
)

target_compile_options(sony_sample PRIVATE -fsigned-char)

# ---- CRSDK header compatibility (GCC 14) ----
# CRSDK uses -1 in enums with CrInt8 underlying type; GCC 14 can error if char is unsigned.
target_compile_options(ccu_daemon PRIVATE -fsigned-char)
target_compile_options(ccu_diag PRIVATE -fsigned-char)
target_compile_options(sony_a74_usb_test PRIVATE -fsigned-char)
target_compile_options(movie_record_test PRIVATE -fsigned-char)
target_compile_options(a74_usb_record_test PRIVATE -fsigned-char)
target_compile_options(a74_usb_settings_test PRIVATE -fsigned-char)
target_compile_options(a74_usb_stills_test PRIVATE -fsigned-char)
target_compile_options(a74_eth_settings_test PRIVATE -fsigned-char)
target_compile_options(a74_eth_stills_test PRIVATE -fsigned-char)
target_compile_options(minimal_test PRIVATE -fsigned-char)
target_compile_options(simple_sdk_test PRIVATE -fsigned-char)
target_compile_options(simple_recording_test PRIVATE -fsigned-char)
target_compile_options(direct_sony_api PRIVATE -fsigned-char)
target_compile_options(remotecli_test PRIVATE -fsigned-char)
target_compile_options(working_sdk_test PRIVATE -fsigned-char)
target_compile_options(sony_a74_direct_api PRIVATE -fsigned-char)

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

# Configure sony_sample library
target_include_directories(sony_sample PRIVATE
  ${CRSDK_ROOT}/app
  ${CRSDK_ROOT}/app/CRSDK
  ${CRSDK_ROOT}/external/opencv/include
  ${CMAKE_SOURCE_DIR}/src
)

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

  target_include_directories(sony_a74_usb_test PRIVATE
    ${CRSDK_ROOT}/app
  )

  target_include_directories(movie_record_test PRIVATE
    ${CRSDK_ROOT}/app
    ${CRSDK_ROOT}/app/CRSDK
    ${CMAKE_SOURCE_DIR}/src
  )

  target_include_directories(a74_usb_record_test PRIVATE
    ${CRSDK_ROOT}/app
    ${CRSDK_ROOT}/app/CRSDK
    ${CMAKE_SOURCE_DIR}/src
  )

  target_include_directories(a74_usb_settings_test PRIVATE
    ${CRSDK_ROOT}/app
    ${CRSDK_ROOT}/app/CRSDK
    ${CMAKE_SOURCE_DIR}/src
  )

  target_include_directories(a74_usb_stills_test PRIVATE
    ${CRSDK_ROOT}/app
    ${CRSDK_ROOT}/app/CRSDK
    ${CMAKE_SOURCE_DIR}/src
  )

  target_include_directories(a74_eth_settings_test PRIVATE
    ${CRSDK_ROOT}/app
    ${CRSDK_ROOT}/app/CRSDK
    ${CMAKE_SOURCE_DIR}/src
  )

  target_include_directories(a74_eth_stills_test PRIVATE
    ${CRSDK_ROOT}/app
    ${CRSDK_ROOT}/app/CRSDK
    ${CMAKE_SOURCE_DIR}/src
  )

  # Configure minimal_test exactly like ccu_diag for debugging
  target_include_directories(minimal_test PRIVATE
    ${CRSDK_ROOT}/app
    ${CRSDK_ROOT}/app/CRSDK
    ${CMAKE_SOURCE_DIR}/src
  )

  # Configure simple_sdk_test to use build directory libraries like RemoteCli
  target_include_directories(simple_sdk_test PRIVATE
    ${CRSDK_ROOT}/app
    ${CRSDK_ROOT}/external/opencv/include
  )

  # Configure remotecli_test exactly like RemoteCli 
  target_include_directories(remotecli_test PRIVATE
    ${CRSDK_ROOT}/app
  )

  # Configure working_sdk_test with OpenCV includes like RemoteCli
  target_include_directories(working_sdk_test PRIVATE
    ${CRSDK_ROOT}/app
    ${CRSDK_ROOT}/external/opencv/include
  )

  # Configure simple_recording_test with OpenCV includes like working_sdk_test
  target_include_directories(simple_recording_test PRIVATE
    ${CRSDK_ROOT}/app
    ${CRSDK_ROOT}/external/opencv/include
  )

  # Configure direct_sony_api with OpenCV includes like RemoteCli
  target_include_directories(direct_sony_api PRIVATE
    ${CRSDK_ROOT}/app
    ${CRSDK_ROOT}/external/opencv/include
  )

  # Configure sony_a74_direct_api with full SDK access
  target_include_directories(sony_a74_direct_api PRIVATE
    ${CRSDK_ROOT}/app
    ${CRSDK_ROOT}/external/opencv/include
  )

  target_link_directories(sony_a74_usb_test PRIVATE
    ${CRSDK_ROOT}/external/crsdk
    ${CRSDK_ROOT}/external/crsdk/CrAdapter
  )

  target_link_directories(movie_record_test PRIVATE
    ${CRSDK_ROOT}/external/crsdk
    ${CRSDK_ROOT}/external/crsdk/CrAdapter
  )

  target_link_directories(a74_usb_record_test PRIVATE
    ${CRSDK_ROOT}/external/crsdk
    ${CRSDK_ROOT}/external/crsdk/CrAdapter
  )

  target_link_directories(a74_usb_settings_test PRIVATE
    ${CRSDK_ROOT}/external/crsdk
    ${CRSDK_ROOT}/external/crsdk/CrAdapter
  )

  target_link_directories(a74_usb_stills_test PRIVATE
    ${CRSDK_ROOT}/external/crsdk
    ${CRSDK_ROOT}/external/crsdk/CrAdapter
  )

  target_link_directories(a74_eth_settings_test PRIVATE
    ${CRSDK_ROOT}/external/crsdk
    ${CRSDK_ROOT}/external/crsdk/CrAdapter
  )

  target_link_directories(a74_eth_stills_test PRIVATE
    ${CRSDK_ROOT}/external/crsdk
    ${CRSDK_ROOT}/external/crsdk/CrAdapter
  )

  target_link_libraries(a74_usb_settings_test PRIVATE
    Cr_Core
    monitor_protocol
    monitor_protocol_pf
    Cr_PTP_USB
    Cr_PTP_IP
    usb-1.0
    ssh2
  )

  target_link_libraries(a74_usb_stills_test PRIVATE
    Cr_Core
    monitor_protocol
    monitor_protocol_pf
    Cr_PTP_USB
    Cr_PTP_IP
    usb-1.0
    ssh2
    pthread
    dl
  )

  target_link_libraries(a74_eth_settings_test PRIVATE
    Cr_Core
    monitor_protocol
    monitor_protocol_pf
    Cr_PTP_USB
    Cr_PTP_IP
    usb-1.0
    ssh2
    pthread
    dl
  )

  target_link_libraries(a74_eth_stills_test PRIVATE
    Cr_Core
    monitor_protocol
    monitor_protocol_pf
    Cr_PTP_USB
    Cr_PTP_IP
    usb-1.0
    ssh2
    pthread
    dl
  )

  target_link_directories(minimal_test PRIVATE
    ${CRSDK_ROOT}/build
  )

  target_link_directories(simple_sdk_test PRIVATE
    ${CRSDK_ROOT}/build
  )

  target_link_directories(remotecli_test PRIVATE
    ${CRSDK_ROOT}/build
  )

  target_link_directories(working_sdk_test PRIVATE
    ${CRSDK_ROOT}/build
  )

  target_link_directories(simple_recording_test PRIVATE
    ${CRSDK_ROOT}/build
  )

  target_link_directories(direct_sony_api PRIVATE
    ${CRSDK_ROOT}/build
  )

  target_link_directories(sony_a74_direct_api PRIVATE
    ${CRSDK_ROOT}/build
  )

  # Add OpenCV library directory
  target_link_directories(remotecli_test PRIVATE
    ${CRSDK_ROOT}/build
  )

  target_link_libraries(sony_a74_usb_test PRIVATE
    Cr_Core
    monitor_protocol
    monitor_protocol_pf
    Cr_PTP_USB
    Cr_PTP_IP
    usb-1.0
    ssh2
    pthread
    dl
  )

  target_link_libraries(movie_record_test PRIVATE
    Cr_Core
    monitor_protocol
    monitor_protocol_pf
    Cr_PTP_USB
    Cr_PTP_IP
    usb-1.0
    ssh2
    pthread
    dl
  )

  target_link_libraries(a74_usb_record_test PRIVATE
    Cr_Core
    monitor_protocol
    monitor_protocol_pf
    Cr_PTP_USB
    Cr_PTP_IP
    usb-1.0
    ssh2
    pthread
    dl
  )

  # Link minimal_test exactly like ccu_diag but use SDK build directory
  target_link_libraries(minimal_test PRIVATE
    Cr_Core
    monitor_protocol
    monitor_protocol_pf
  )

  # Link simple_sdk_test with just core libraries like RemoteCli
  target_link_libraries(simple_sdk_test PRIVATE
    Cr_Core
    ${CRSDK_ROOT}/build/libopencv_core.so.408
    ${CRSDK_ROOT}/build/libopencv_imgcodecs.so.408
    ${CRSDK_ROOT}/build/libopencv_imgproc.so.408
  )

  # Create imported targets for OpenCV libraries
  add_library(opencv_core SHARED IMPORTED)
  set_target_properties(opencv_core PROPERTIES
    IMPORTED_LOCATION ${CRSDK_ROOT}/build/libopencv_core.so.408
  )
  
  add_library(opencv_imgcodecs SHARED IMPORTED)
  set_target_properties(opencv_imgcodecs PROPERTIES
    IMPORTED_LOCATION ${CRSDK_ROOT}/build/libopencv_imgcodecs.so.408
  )
  
  add_library(opencv_imgproc SHARED IMPORTED)
  set_target_properties(opencv_imgproc PROPERTIES
    IMPORTED_LOCATION ${CRSDK_ROOT}/build/libopencv_imgproc.so.408
  )

  # Link remotecli_test with Cr_Core and OpenCV libraries like RemoteCli
  target_link_libraries(remotecli_test PRIVATE
    Cr_Core
    opencv_core
    opencv_imgcodecs  
    opencv_imgproc
  )

  # Link working_sdk_test with OpenCV like RemoteCli
  target_link_libraries(working_sdk_test PRIVATE
    Cr_Core
    opencv_core
    opencv_imgcodecs
    opencv_imgproc
  )

  # Link simple_recording_test with OpenCV like working_sdk_test
  target_link_libraries(simple_recording_test PRIVATE
    Cr_Core
    opencv_core
    opencv_imgcodecs
    opencv_imgproc
  )

  # Link direct_sony_api with full SDK capabilities
  target_link_libraries(direct_sony_api PRIVATE
    Cr_Core
    opencv_core
    opencv_imgcodecs
    opencv_imgproc
  )

  # Link sony_a74_direct_api with full SDK capabilities
  target_link_libraries(sony_a74_direct_api PRIVATE
    Cr_Core
    opencv_core
    opencv_imgcodecs
    opencv_imgproc
  )

  set_target_properties(ccu_diag PROPERTIES
    BUILD_RPATH "${CRSDK_ROOT}/external/crsdk;${CRSDK_ROOT}/external/crsdk/CrAdapter"
    INSTALL_RPATH "${CRSDK_ROOT}/external/crsdk;${CRSDK_ROOT}/external/crsdk/CrAdapter"
  )

  set_target_properties(sony_a74_usb_test PROPERTIES
    BUILD_RPATH "${CRSDK_ROOT}/external/crsdk;${CRSDK_ROOT}/external/crsdk/CrAdapter"
    INSTALL_RPATH "${CRSDK_ROOT}/external/crsdk;${CRSDK_ROOT}/external/crsdk/CrAdapter"
  )

  set_target_properties(movie_record_test PROPERTIES
    BUILD_RPATH "${CRSDK_ROOT}/external/crsdk;${CRSDK_ROOT}/external/crsdk/CrAdapter"
    INSTALL_RPATH "${CRSDK_ROOT}/external/crsdk;${CRSDK_ROOT}/external/crsdk/CrAdapter"
  )

  set_target_properties(a74_usb_record_test PROPERTIES
    BUILD_RPATH "${CRSDK_ROOT}/external/crsdk;${CRSDK_ROOT}/external/crsdk/CrAdapter"
    INSTALL_RPATH "${CRSDK_ROOT}/external/crsdk;${CRSDK_ROOT}/external/crsdk/CrAdapter"
  )

  set_target_properties(minimal_test PROPERTIES
    BUILD_RPATH "${CRSDK_ROOT}/build"
    INSTALL_RPATH "${CRSDK_ROOT}/build"
  )

  set_target_properties(simple_sdk_test PROPERTIES
    BUILD_RPATH "${CRSDK_ROOT}/build"
    INSTALL_RPATH "${CRSDK_ROOT}/build"
  )

  set_target_properties(remotecli_test PROPERTIES
    BUILD_RPATH "${CRSDK_ROOT}/build"
    INSTALL_RPATH "${CRSDK_ROOT}/build"
  )

  set_target_properties(working_sdk_test PROPERTIES
    BUILD_RPATH "${CRSDK_ROOT}/build"
    INSTALL_RPATH "${CRSDK_ROOT}/build"
  )

  set_target_properties(simple_recording_test PROPERTIES
    BUILD_RPATH "${CRSDK_ROOT}/build"
    INSTALL_RPATH "${CRSDK_ROOT}/build"
  )

  set_target_properties(direct_sony_api PROPERTIES
    BUILD_RPATH "${CRSDK_ROOT}/build"
    INSTALL_RPATH "${CRSDK_ROOT}/build"
  )

  set_target_properties(sony_a74_direct_api PROPERTIES
    BUILD_RPATH "${CRSDK_ROOT}/build"
    INSTALL_RPATH "${CRSDK_ROOT}/build"
  )
endif()
add_executable(ccu_cli
  tools/ccu_cli.cpp
  src/protocol.cpp
)

target_link_libraries(ccu_cli PRIVATE pthread)

# ---- Camera Control Test ----
add_executable(camera_control_test
  src/camera_control_test.cpp
  src/camera_controller.cpp
  src/protocol.cpp
)

target_compile_options(camera_control_test PRIVATE -fsigned-char)

if (CRSDK_ROOT)
  target_include_directories(camera_control_test PRIVATE
    ${CRSDK_ROOT}/app
    ${CRSDK_ROOT}/app/CRSDK
    ${CRSDK_ROOT}/external/opencv/include
    ${CMAKE_SOURCE_DIR}/src
  )

  target_link_directories(camera_control_test PRIVATE
    ${CRSDK_ROOT}/external/crsdk
    ${CRSDK_ROOT}/external/crsdk/CrAdapter
  )

  target_link_libraries(camera_control_test PRIVATE
    Cr_Core
    monitor_protocol
    monitor_protocol_pf
    Cr_PTP_USB
    Cr_PTP_IP
    usb-1.0
    ssh2
    pthread
    dl
  )

  set_target_properties(camera_control_test PROPERTIES
    BUILD_RPATH "${CRSDK_ROOT}/external/crsdk;${CRSDK_ROOT}/external/crsdk/CrAdapter"
    INSTALL_RPATH "${CRSDK_ROOT}/external/crsdk;${CRSDK_ROOT}/external/crsdk/CrAdapter"
  )
endif()

# ---- Simple Camera Test ----
add_executable(simple_camera_test
  src/simple_camera_test.cpp
)

target_compile_options(simple_camera_test PRIVATE -fsigned-char)

if (CRSDK_ROOT)
  target_include_directories(simple_camera_test PRIVATE
    ${CRSDK_ROOT}/app
    ${CRSDK_ROOT}/app/CRSDK
  )

  target_link_directories(simple_camera_test PRIVATE
    ${CRSDK_ROOT}/external/crsdk
    ${CRSDK_ROOT}/external/crsdk/CrAdapter
  )

  target_link_libraries(simple_camera_test PRIVATE
    Cr_Core
    monitor_protocol
    monitor_protocol_pf
    Cr_PTP_USB
    Cr_PTP_IP
    usb-1.0
    ssh2
    pthread
    dl
  )

  set_target_properties(simple_camera_test PROPERTIES
    BUILD_RPATH "${CRSDK_ROOT}/external/crsdk;${CRSDK_ROOT}/external/crsdk/CrAdapter"
    INSTALL_RPATH "${CRSDK_ROOT}/external/crsdk;${CRSDK_ROOT}/external/crsdk/CrAdapter"
  )
endif()

# ---- Connection Test ----
add_executable(connection_test
  src/connection_test.cpp
)

target_compile_options(connection_test PRIVATE -fsigned-char)

if (CRSDK_ROOT)
  target_include_directories(connection_test PRIVATE
    ${CRSDK_ROOT}/app
    ${CRSDK_ROOT}/app/CRSDK
  )

  target_link_directories(connection_test PRIVATE
    ${CRSDK_ROOT}/external/crsdk
    ${CRSDK_ROOT}/external/crsdk/CrAdapter
  )

  target_link_libraries(connection_test PRIVATE
    Cr_Core
    monitor_protocol
    monitor_protocol_pf
    Cr_PTP_USB
    Cr_PTP_IP
    usb-1.0
    ssh2
    pthread
    dl
  )

  set_target_properties(connection_test PROPERTIES
    BUILD_RPATH "${CRSDK_ROOT}/external/crsdk;${CRSDK_ROOT}/external/crsdk/CrAdapter"
    INSTALL_RPATH "${CRSDK_ROOT}/external/crsdk;${CRSDK_ROOT}/external/crsdk/CrAdapter"
  )

# ---- Sony Test using Direct SDK ----
add_executable(sony_test
  src/sony_test.cpp
)

target_compile_options(sony_test PRIVATE -fsigned-char)

  target_include_directories(sony_test PRIVATE
    ${CRSDK_ROOT}/app
    ${CRSDK_ROOT}/app/CRSDK
    ${CMAKE_SOURCE_DIR}/src
  )

  target_link_directories(sony_test PRIVATE
    ${CRSDK_ROOT}/external/crsdk
    ${CRSDK_ROOT}/external/crsdk/CrAdapter
  )

  target_link_libraries(sony_test PRIVATE
    Cr_Core
    monitor_protocol
    monitor_protocol_pf
    Cr_PTP_USB
    Cr_PTP_IP
    usb-1.0
    ssh2
    pthread
    dl
  )

  set_target_properties(sony_test PROPERTIES
    BUILD_RPATH "${CRSDK_ROOT}/external/crsdk;${CRSDK_ROOT}/external/crsdk/CrAdapter"
    INSTALL_RPATH "${CRSDK_ROOT}/external/crsdk;${CRSDK_ROOT}/external/crsdk/CrAdapter"
  )

# ---- Fingerprint Authentication Test ----
add_executable(fingerprint_auth_test
  src/fingerprint_auth_test.cpp
)

target_compile_options(fingerprint_auth_test PRIVATE -fsigned-char)

  target_include_directories(fingerprint_auth_test PRIVATE
    ${CRSDK_ROOT}/app
    ${CRSDK_ROOT}/app/CRSDK
    ${CMAKE_SOURCE_DIR}/src
  )

  target_link_directories(fingerprint_auth_test PRIVATE
    ${CRSDK_ROOT}/external/crsdk
    ${CRSDK_ROOT}/external/crsdk/CrAdapter
  )

  target_link_libraries(fingerprint_auth_test PRIVATE
    Cr_Core
    monitor_protocol
    monitor_protocol_pf
    Cr_PTP_USB
    Cr_PTP_IP
    usb-1.0
    ssh2
    pthread
    dl
  )

  set_target_properties(fingerprint_auth_test PROPERTIES
    BUILD_RPATH "${CRSDK_ROOT}/external/crsdk;${CRSDK_ROOT}/external/crsdk/CrAdapter"
    INSTALL_RPATH "${CRSDK_ROOT}/external/crsdk;${CRSDK_ROOT}/external/crsdk/CrAdapter"
  )

# ---- Camera Authentication Success Test (with correct fingerprint) ----
add_executable(camera_auth_success_test
  src/camera_auth_success_test.cpp
)

target_compile_options(camera_auth_success_test PRIVATE -fsigned-char)

  target_include_directories(camera_auth_success_test PRIVATE
    ${CRSDK_ROOT}/app
    ${CRSDK_ROOT}/app/CRSDK
    ${CMAKE_SOURCE_DIR}/src
  )

  target_link_directories(camera_auth_success_test PRIVATE
    ${CRSDK_ROOT}/external/crsdk
    ${CRSDK_ROOT}/external/crsdk/CrAdapter
  )

  target_link_libraries(camera_auth_success_test PRIVATE
    Cr_Core
    monitor_protocol
    monitor_protocol_pf
    Cr_PTP_USB
    Cr_PTP_IP
    usb-1.0
    ssh2
    pthread
    dl
  )

  set_target_properties(camera_auth_success_test PROPERTIES
    BUILD_RPATH "${CRSDK_ROOT}/external/crsdk;${CRSDK_ROOT}/external/crsdk/CrAdapter"
    INSTALL_RPATH "${CRSDK_ROOT}/external/crsdk;${CRSDK_ROOT}/external/crsdk/CrAdapter"
  )

# ---- Detailed Authentication Analysis ----
add_executable(detailed_auth_test
  src/detailed_auth_test.cpp
)

target_compile_options(detailed_auth_test PRIVATE -fsigned-char)

  target_include_directories(detailed_auth_test PRIVATE
    ${CRSDK_ROOT}/app
    ${CRSDK_ROOT}/app/CRSDK
    ${CMAKE_SOURCE_DIR}/src
  )

  target_link_directories(detailed_auth_test PRIVATE
    ${CRSDK_ROOT}/external/crsdk
    ${CRSDK_ROOT}/external/crsdk/CrAdapter
  )

  target_link_libraries(detailed_auth_test PRIVATE
    Cr_Core
    monitor_protocol
    monitor_protocol_pf
    Cr_PTP_USB
    Cr_PTP_IP
    usb-1.0
    ssh2
    pthread
    dl
  )

  set_target_properties(detailed_auth_test PROPERTIES
    BUILD_RPATH "${CRSDK_ROOT}/external/crsdk;${CRSDK_ROOT}/external/crsdk/CrAdapter"
    INSTALL_RPATH "${CRSDK_ROOT}/external/crsdk;${CRSDK_ROOT}/external/crsdk/CrAdapter"
  )

# ---- Direct Recording Test ----
add_executable(direct_record
  src/direct_record.cpp
)

target_compile_options(direct_record PRIVATE -fsigned-char)

  target_include_directories(direct_record PRIVATE
    ${CRSDK_ROOT}/app
    ${CRSDK_ROOT}/app/CRSDK
    ${CMAKE_SOURCE_DIR}/src
  )

  target_link_directories(direct_record PRIVATE
    ${CRSDK_ROOT}/external/crsdk
    ${CRSDK_ROOT}/external/crsdk/CrAdapter
  )

  target_link_libraries(direct_record PRIVATE
    Cr_Core
    monitor_protocol
    monitor_protocol_pf
    Cr_PTP_USB
    Cr_PTP_IP
    usb-1.0
    ssh2
    pthread
    dl
  )

  set_target_properties(direct_record PROPERTIES
    BUILD_RPATH "${CRSDK_ROOT}/external/crsdk;${CRSDK_ROOT}/external/crsdk/CrAdapter"
    INSTALL_RPATH "${CRSDK_ROOT}/external/crsdk;${CRSDK_ROOT}/external/crsdk/CrAdapter"
  )
endif()
