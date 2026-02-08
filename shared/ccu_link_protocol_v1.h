#pragma once
// ============================================================
// CCU <-> PI LINK PROTOCOL (AUTHORITATIVE CONTRACT)
// v1.0
//
// This file is the single source of truth for the CCU <-> Pi
// communication interface.
//
// RULES:
//  - CCU code may include this file directly.
//  - Pi code must use an identical copy of this file.
//  - Do NOT diverge definitions between sides.
//  - Version bump required for incompatible changes.
// ============================================================

#define CCU_PI_PROTOCOL_VERSION 0x00010000

// ------------------------------------------------------------
// Camera targets
// ------------------------------------------------------------
typedef enum {
  CAM_A   = 0,
  CAM_B   = 1,
  CAM_C   = 2,
  CAM_D   = 3,
  CAM_E   = 4,
  CAM_F   = 5,
  CAM_G   = 6,
  CAM_H   = 7,
  CAM_ALL = 255
} ccu_cam_target_t;

// ------------------------------------------------------------
// Command IDs (CCU -> Pi)
// ------------------------------------------------------------
typedef enum {
  PI_CMD_NOP = 0,

  PI_CMD_REC_SET,        // arg: 0/1
  PI_CMD_REC_TOGGLE,

  PI_CMD_ISO_SET,        // arg: iso value
  PI_CMD_WB_SET,         // arg: kelvin
  PI_CMD_SHUTTER_SET,    // arg: implementation-defined
  PI_CMD_FPS_SET,

  PI_CMD_STATUS_REQUEST  // request full state push
} ccu_pi_cmd_t;

// ------------------------------------------------------------
// Result / ACK codes (Pi -> CCU)
// ------------------------------------------------------------
typedef enum {
  PI_ACK_OK = 0,
  PI_ACK_ERR_UNSUPPORTED,
  PI_ACK_ERR_BAD_ARG,
  PI_ACK_ERR_BUSY,
  PI_ACK_ERR_INTERNAL
} ccu_pi_ack_t;

// ------------------------------------------------------------
// Status flags (Pi -> CCU)
// ------------------------------------------------------------
typedef enum {
  PI_STATUS_IDLE    = 0,
  PI_STATUS_RECORD  = 1 << 0,
  PI_STATUS_ERROR   = 1 << 7
} ccu_pi_status_flags_t;
