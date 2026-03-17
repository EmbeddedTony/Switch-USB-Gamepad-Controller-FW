#ifndef _TUSB_CONFIG_H_
#define _TUSB_CONFIG_H_

#ifdef __cplusplus
extern "C" {
#endif

// ---- MCU selection ----
#define CFG_TUSB_MCU   OPT_MCU_STM32H5

// ---- Root hub port 0 = Device mode, Full Speed ----
#define CFG_TUSB_RHPORT0_MODE  OPT_MODE_DEVICE

// ---- RTOS (none = bare metal) ----
#define CFG_TUSB_OS    OPT_OS_NONE

// ---- Debug ----
#define CFG_TUSB_DEBUG 0

// ---- Endpoint0 max packet size (must be 64 for Switch) ----
#define CFG_TUD_ENDPOINT0_SIZE  64

// ---- Device stack ----
#define CFG_TUD_ENABLED  1

// ---- HID: 1 interface (Pro Controller) ----
#define CFG_TUD_HID      1
#define CFG_TUD_HID_EP_BUFSIZE  64

// Disable unused classes
#define CFG_TUD_CDC      0
#define CFG_TUD_MSC      0
#define CFG_TUD_MIDI     0
#define CFG_TUD_VENDOR   0

#ifdef __cplusplus
}
#endif
#endif
