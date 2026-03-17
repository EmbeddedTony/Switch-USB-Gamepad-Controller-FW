#ifndef USB_DESCRIPTORS_H_
#define USB_DESCRIPTORS_H_

#include <stdint.h>
#include <stdbool.h>

// ---- Nintendo Switch Pro Controller Protocol ----

#define SWITCH_PRO_VENDOR_ID     0x057E
#define SWITCH_PRO_PRODUCT_ID    0x2009
#define SWITCH_PRO_ENDPOINT_SIZE 64

// ---- Report IDs ----
// Input (device → host)
#define REPORT_ID_INPUT_30   0x30  // Standard full input report
#define REPORT_ID_INPUT_21   0x21  // Subcommand reply
#define REPORT_ID_INPUT_81   0x81  // 0x80 command reply

// Output (host → device)
#define REPORT_ID_OUTPUT_01  0x01  // UART subcommand
#define REPORT_ID_OUTPUT_10  0x10  // Rumble only
#define REPORT_ID_OUTPUT_80  0x80  // Config/handshake command

// ---- 0x80 Config Sub-commands ----
#define SUBCMD_80_IDENTIFY            0x01
#define SUBCMD_80_HANDSHAKE           0x02
#define SUBCMD_80_BAUD_RATE           0x03
#define SUBCMD_80_DISABLE_USB_TIMEOUT 0x04
#define SUBCMD_80_ENABLE_USB_TIMEOUT  0x05

// ---- 0x01 UART Sub-commands ----
#define SUBCMD_GET_STATE          0x00
#define SUBCMD_BT_PAIR            0x01
#define SUBCMD_DEVICE_INFO        0x02
#define SUBCMD_SET_MODE           0x03
#define SUBCMD_TRIGGER_BUTTONS    0x04
#define SUBCMD_SET_SHIPMENT       0x08
#define SUBCMD_SPI_READ           0x10
#define SUBCMD_SET_NFC_IR_CONFIG  0x21
#define SUBCMD_SET_NFC_IR_STATE   0x22
#define SUBCMD_SET_PLAYER_LIGHTS  0x30
#define SUBCMD_GET_PLAYER_LIGHTS  0x31
#define SUBCMD_SET_HOME_LIGHT     0x38
#define SUBCMD_TOGGLE_IMU         0x40
#define SUBCMD_IMU_SENSITIVITY    0x41
#define SUBCMD_ENABLE_VIBRATION   0x48

// ---- Pro Controller Input Report (0x30) ----
// 3-byte packed 12-bit analog sticks
typedef struct __attribute__((packed)) {
    uint8_t data[3];
} switch_analog_t;

static inline void switch_analog_set_xy(switch_analog_t *a, uint16_t x, uint16_t y) {
    a->data[0] = (uint8_t)(x & 0xFF);
    a->data[1] = (uint8_t)(((x >> 8) & 0x0F) | ((y & 0x0F) << 4));
    a->data[2] = (uint8_t)((y >> 4) & 0xFF);
}

typedef struct __attribute__((packed)) {
    uint8_t connection_info : 4;
    uint8_t battery_level   : 4;

    // byte 0: right-side buttons + triggers
    uint8_t btn_y       : 1;
    uint8_t btn_x       : 1;
    uint8_t btn_b       : 1;
    uint8_t btn_a       : 1;
    uint8_t btn_rsr     : 1;  // Right SR (JoyCon)
    uint8_t btn_rsl     : 1;  // Right SL (JoyCon)
    uint8_t btn_r       : 1;
    uint8_t btn_zr      : 1;

    // byte 1: shared buttons
    uint8_t btn_minus   : 1;
    uint8_t btn_plus    : 1;
    uint8_t btn_rstick  : 1;
    uint8_t btn_lstick  : 1;
    uint8_t btn_home    : 1;
    uint8_t btn_capture : 1;
    uint8_t _pad0       : 1;
    uint8_t charging    : 1;

    // byte 2: left-side buttons + triggers
    uint8_t dpad_down   : 1;
    uint8_t dpad_up     : 1;
    uint8_t dpad_right  : 1;
    uint8_t dpad_left   : 1;
    uint8_t btn_lsr     : 1;  // Left SR (JoyCon)
    uint8_t btn_lsl     : 1;  // Left SL (JoyCon)
    uint8_t btn_l       : 1;
    uint8_t btn_zl      : 1;

    switch_analog_t left_stick;
    switch_analog_t right_stick;
} switch_input_report_t;

// Full 0x30 input report (64 bytes)
typedef struct __attribute__((packed)) {
    uint8_t report_id;              // 0x30
    uint8_t timestamp;              // incrementing counter
    switch_input_report_t input;
    uint8_t rumble_report;          // vibrator input report
    uint8_t imu_data[36];           // 3 IMU samples (empty)
    uint8_t padding[15];            // zero padding to 64 bytes
} switch_pro_report_t;

// ---- Protocol state ----
typedef struct {
    bool     handshake_done;        // 0x80 handshake complete
    bool     reports_enabled;       // Switch asked for input reports
    uint8_t  report_counter;        // incrementing timestamp for 0x30
    uint8_t  report_buf[64];        // scratch buffer for responses
    uint8_t  player_id;             // player LED state
} switch_pro_state_t;

extern switch_pro_state_t pro_state;

// Process an incoming report from the Switch
void switch_pro_handle_output(const uint8_t *data, uint16_t len);

#endif
