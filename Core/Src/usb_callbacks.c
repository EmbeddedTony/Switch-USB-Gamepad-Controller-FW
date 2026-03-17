#include "tusb.h"
#include "usb_descriptors.h"
#include "debug_uart.h"
#include <string.h>

// ---- Pro Controller protocol state ----
switch_pro_state_t pro_state;

// ---- SPI Flash Data (factory calibration / config) ----
// Based on GP2040-CE and SwitchDualShockAdapter — addresses the Switch reads

// 0x6000: Serial number area (16 bytes of 0xFF = no serial)
static const uint8_t spi_6000[] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    // +0x10: padding
    0xFF, 0xFF,
    // +0x12: device type (0x03 = Pro Controller)
    0x03,
    // +0x13: unknown (usually 0xA0)
    0xA0,
    // +0x14..0x1A: unknown
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    // +0x1B: color info (0x02 = has grip colors)
    0x02,
    // +0x1C..0x1F: unknown
    0xFF, 0xFF, 0xFF, 0xFF,
    // +0x20..0x37: left stick factory calibration config/params
    0xE3, 0xFF, 0x39, 0xFF, 0xED, 0x01, 0x00, 0x40,
    0x00, 0x40, 0x00, 0x40, 0x09, 0x00, 0xEA, 0xFF,
    0xA1, 0xFF, 0x3B, 0x34, 0x3B, 0x34, 0x3B, 0x34,
    // +0x38..0x3C
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    // +0x3D..0x45: left stick user calibration (9 bytes)
    0xA4, 0x46, 0x6A, 0x00, 0x08, 0x80, 0xA4, 0x46, 0x6A,
    // +0x46..0x4E: right stick user calibration (9 bytes)
    0x00, 0x08, 0x80, 0xA4, 0x46, 0x6A, 0xA4, 0x46, 0x6A,
    // +0x4F
    0xFF,
    // +0x50..0x52: body color (dark grey)
    0x1B, 0x1B, 0x1D,
    // +0x53..0x55: button color (white)
    0xFF, 0xFF, 0xFF,
    // +0x56..0x58: left grip color
    0xEC, 0x00, 0x8C,
    // +0x59..0x5B: right grip color
    0xEC, 0x00, 0x8C,
    // +0x5C
    0x01
};

// 0x6080: Stick parameters (factory) — left stick
static const uint8_t spi_6080[] = {
    0x50, 0xFD, 0x00, 0x00, 0xC6, 0x0F,
    0x0F, 0x30, 0x61, 0xAE, 0x90, 0xD9, 0xD4, 0x14,
    0x54, 0x41, 0x15, 0x54, 0xC7, 0x79, 0x9C, 0x33,
    0x36, 0x63
};

// 0x6098: Stick parameters (factory) — right stick
static const uint8_t spi_6098[] = {
    0x0F, 0x30, 0x61, 0xAE, 0x90, 0xD9, 0xD4, 0x14,
    0x54, 0x41, 0x15, 0x54, 0xC7, 0x79, 0x9C, 0x33,
    0x36, 0x63
};

// 0x8010: User calibration area (all 0xFF = no user calibration)
static const uint8_t spi_8010[] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xB2, 0xA1
};

// 0x8028: User stick calibration
static const uint8_t spi_8028[] = {
    0xBE, 0xFF, 0x3E, 0x00, 0xF0, 0x01,
    0x00, 0x40, 0x00, 0x40, 0x00, 0x40,
    0xFE, 0xFF, 0xFE, 0xFF,
    0x08, 0x00, 0xE7, 0x3B, 0xE7, 0x3B, 0xE7, 0x3B
};

// ---- Device Info ----
static const uint8_t device_info[] = {
    0x03, 0x48,  // FW version
    0x03,        // Controller type: Pro Controller
    0x02,        // Unknown
    0xC7, 0xA3, 0x22, 0x53, 0x23, 0x43,  // MAC address (reversed)
    0x03,        // Unknown
    0x01         // Use colors
};

// MAC address for identify command (normal order)
static const uint8_t mac_address[] = { 0x43, 0x23, 0x53, 0x22, 0xA3, 0xC7 };

// ---- Helper: read SPI flash data ----
static void read_spi_flash(uint8_t *dest, uint32_t address, uint8_t size) {
    // Try to serve from our static data tables
    // We handle reads by base address range
    const uint8_t *src = NULL;
    uint32_t src_size = 0;
    uint32_t base = 0;

    if (address >= 0x6000 && address < 0x6000 + sizeof(spi_6000)) {
        src = spi_6000;
        src_size = sizeof(spi_6000);
        base = 0x6000;
    } else if (address >= 0x6080 && address < 0x6080 + sizeof(spi_6080)) {
        src = spi_6080;
        src_size = sizeof(spi_6080);
        base = 0x6080;
    } else if (address >= 0x6098 && address < 0x6098 + sizeof(spi_6098)) {
        src = spi_6098;
        src_size = sizeof(spi_6098);
        base = 0x6098;
    } else if (address >= 0x8010 && address < 0x8010 + sizeof(spi_8010)) {
        src = spi_8010;
        src_size = sizeof(spi_8010);
        base = 0x8010;
    } else if (address >= 0x8028 && address < 0x8028 + sizeof(spi_8028)) {
        src = spi_8028;
        src_size = sizeof(spi_8028);
        base = 0x8028;
    }

    if (src != NULL) {
        uint32_t offset = address - base;
        uint8_t avail = (uint8_t)(src_size - offset);
        uint8_t copy_size = (size < avail) ? size : avail;
        memcpy(dest, src + offset, copy_size);
        if (copy_size < size) {
            memset(dest + copy_size, 0xFF, size - copy_size);
        }
    } else {
        // Unknown address — return 0xFF (no data)
        memset(dest, 0xFF, size);
    }
}

// ---- Helper: fill input sub-report into buffer (neutral position) ----
static void fill_input_subreport(uint8_t *buf) {
    // 11 bytes of switch_input_report_t at neutral
    memset(buf, 0, 11);
    buf[0] = 0x08;  // battery level = 8 (good), connection = 0 (USB)

    // Sticks at center: 0x7FF for both X and Y (12-bit)
    // Byte layout: [0]=batt/conn, [1..3]=buttons, [4..6]=left stick, [7..9]=right stick
    // 0x7FF packed: {0xFF, 0xF7, 0x7F} matching GP2040-CE
    buf[4] = 0xFF; buf[5] = 0xF7; buf[6] = 0x7F;
    buf[7] = 0xFF; buf[8] = 0xF7; buf[9] = 0x7F;
}

// ---- Queue a response report ----
static bool report_queued = false;
static uint8_t queued_report_id = 0;

static void queue_response(uint8_t report_id) {
    queued_report_id = report_id;
    report_queued = true;
}

// ---- Handle 0x80 config commands ----
static void handle_config_command(uint8_t subcmd) {
    uint8_t *buf = pro_state.report_buf;
    memset(buf, 0, 64);

    debug_print("[80] cmd="); debug_hex8(subcmd); debug_print("\r\n");

    switch (subcmd) {
    case SUBCMD_80_IDENTIFY:
        // Reply: 0x81 0x01 + controller type + MAC
        buf[0] = REPORT_ID_INPUT_81;
        buf[1] = SUBCMD_80_IDENTIFY;
        buf[2] = 0x00;
        buf[3] = 0x03;  // Pro Controller
        memcpy(&buf[4], mac_address, 6);
        queue_response(0);
        break;

    case SUBCMD_80_HANDSHAKE:
        buf[0] = REPORT_ID_INPUT_81;
        buf[1] = SUBCMD_80_HANDSHAKE;
        queue_response(0);
        break;

    case SUBCMD_80_BAUD_RATE:
        buf[0] = REPORT_ID_INPUT_81;
        buf[1] = SUBCMD_80_BAUD_RATE;
        queue_response(0);
        break;

    case SUBCMD_80_DISABLE_USB_TIMEOUT:
        // This means "switch to full reporting mode"
        debug_println("[80] READY - reports enabled");
        pro_state.handshake_done = true;
        pro_state.reports_enabled = true;
        buf[0] = REPORT_ID_INPUT_30;
        buf[1] = subcmd;
        queue_response(0);
        break;

    case SUBCMD_80_ENABLE_USB_TIMEOUT:
        buf[0] = REPORT_ID_INPUT_30;
        buf[1] = subcmd;
        queue_response(0);
        break;

    default:
        buf[0] = REPORT_ID_INPUT_30;
        buf[1] = subcmd;
        queue_response(0);
        break;
    }
}

// ---- Handle 0x01 UART subcommands ----
static void handle_subcommand(const uint8_t *data, uint16_t len) {
    uint8_t *buf = pro_state.report_buf;
    memset(buf, 0, 64);

    // 0x21 reply frame:
    // [0] = 0x21 (report ID)
    // [1] = timestamp
    // [2..12] = input sub-report (11 bytes)
    // [13] = ACK byte (0x80 | subcmd if data follows, 0x80 if ACK only)
    // [14] = subcmd echo
    // [15+] = reply data

    buf[0] = REPORT_ID_INPUT_21;
    buf[1] = pro_state.report_counter++;
    fill_input_subreport(&buf[2]);

    uint8_t subcmd = data[10]; // subcommand ID is at offset 10

    debug_print("[01] sub="); debug_hex8(subcmd); debug_print("\r\n");

    switch (subcmd) {
    case SUBCMD_GET_STATE:
        buf[13] = 0x80 | subcmd;
        buf[14] = subcmd;
        buf[15] = 0x03;
        break;

    case SUBCMD_BT_PAIR:
        buf[13] = 0x80 | subcmd;
        buf[14] = subcmd;
        buf[15] = 0x03;
        buf[16] = 0x01;
        break;

    case SUBCMD_DEVICE_INFO:
        buf[13] = 0x82;
        buf[14] = SUBCMD_DEVICE_INFO;
        memcpy(&buf[15], device_info, sizeof(device_info));
        break;

    case SUBCMD_SPI_READ: {
        // data[11..14] = address (little-endian 32-bit)
        // data[15] = size
        uint32_t addr = (uint32_t)data[11] | ((uint32_t)data[12] << 8) |
                        ((uint32_t)data[13] << 16) | ((uint32_t)data[14] << 24);
        uint8_t size = data[15];
        debug_print("[SPI] addr="); debug_hex32(addr); debug_print(" sz="); debug_hex8(size); debug_print("\r\n");

        buf[13] = 0x90;
        buf[14] = subcmd;
        // Echo back the address and size
        buf[15] = data[11];
        buf[16] = data[12];
        buf[17] = data[13];
        buf[18] = data[14];
        buf[19] = size;
        // Read SPI flash data into buf[20..]
        if (size > 44) size = 44;  // clamp to prevent overflow
        read_spi_flash(&buf[20], addr, size);
        break;
    }

    case SUBCMD_SET_MODE:
        buf[13] = 0x80;
        buf[14] = subcmd;
        break;

    case SUBCMD_TRIGGER_BUTTONS:
        buf[13] = 0x83;
        buf[14] = SUBCMD_TRIGGER_BUTTONS;
        break;

    case SUBCMD_SET_PLAYER_LIGHTS:
        pro_state.player_id = data[11];
        buf[13] = 0x80;
        buf[14] = subcmd;
        break;

    case SUBCMD_GET_PLAYER_LIGHTS:
        buf[13] = 0xB1;
        buf[14] = subcmd;
        buf[15] = pro_state.player_id;
        break;

    case SUBCMD_TOGGLE_IMU:
    case SUBCMD_IMU_SENSITIVITY:
    case SUBCMD_ENABLE_VIBRATION:
    case SUBCMD_SET_SHIPMENT:
    case SUBCMD_SET_NFC_IR_CONFIG:
    case SUBCMD_SET_NFC_IR_STATE:
    case SUBCMD_SET_HOME_LIGHT:
        // Simple ACK — byte [13] must be 0x80, NOT 0x80|subcmd
        buf[13] = 0x80;
        buf[14] = subcmd;
        break;

    default:
        // Unknown subcommand — plain ACK
        buf[13] = 0x80;
        buf[14] = subcmd;
        break;
    }

    debug_print("  -> ACK="); debug_hex8(buf[13]);
    debug_print(" sub="); debug_hex8(buf[14]); debug_print("\r\n");
    queue_response(0);
}

// ---- Public: process incoming output report from Switch ----
void switch_pro_handle_output(const uint8_t *data, uint16_t len) {
    if (len < 2) return;

    debug_dump("<< IN", data, (len > 20) ? 20 : len);

    uint8_t report_id = data[0];
    uint8_t sub_id    = data[1];

    switch (report_id) {
    case REPORT_ID_OUTPUT_80:
        handle_config_command(sub_id);
        break;
    case REPORT_ID_OUTPUT_01:
        if (len >= 11) {
            handle_subcommand(data, len);
        }
        break;
    case REPORT_ID_OUTPUT_10:
        // Rumble-only packet — ignore
        break;
    default:
        break;
    }
}

// ---- TinyUSB callbacks ----
void tud_mount_cb(void) {
    debug_println("[USB] MOUNTED");
    pro_state.handshake_done = false;
    pro_state.reports_enabled = false;
    pro_state.report_counter = 0;
    report_queued = false;
}

void tud_umount_cb(void) {
    debug_println("[USB] UNMOUNTED");
    pro_state.handshake_done = false;
    pro_state.reports_enabled = false;
}

void tud_suspend_cb(bool remote_wakeup_en) { (void)remote_wakeup_en; }
void tud_resume_cb(void) {}

// GET_REPORT — return empty/neutral
uint16_t tud_hid_get_report_cb(uint8_t itf, uint8_t report_id,
                                hid_report_type_t report_type,
                                uint8_t* buffer, uint16_t reqlen) {
    (void)itf; (void)report_id; (void)report_type; (void)reqlen;
    return 0;
}

// SET_REPORT — Switch sends commands via the OUT endpoint
void tud_hid_set_report_cb(uint8_t itf, uint8_t report_id,
                            hid_report_type_t report_type,
                            uint8_t const* buffer, uint16_t bufsize) {
    (void)itf; (void)report_id; (void)report_type;
    switch_pro_handle_output(buffer, bufsize);
}

// ---- Called from main loop to send queued responses ----
bool switch_pro_send_queued(void) {
    if (!report_queued) return false;
    if (!tud_hid_ready()) return false;

    debug_dump(">> OUT", pro_state.report_buf, 20);
    tud_hid_report(queued_report_id, pro_state.report_buf, 64);
    report_queued = false;
    return true;
}
