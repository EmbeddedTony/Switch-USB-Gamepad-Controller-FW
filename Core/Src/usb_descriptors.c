#include "tusb.h"
#include "usb_descriptors.h"
#include <string.h>

// ---- Device Descriptor ----
// Nintendo Switch Pro Controller
tusb_desc_device_t const desc_device = {
  .bLength            = sizeof(tusb_desc_device_t),
  .bDescriptorType    = TUSB_DESC_DEVICE,
  .bcdUSB             = 0x0200,
  .bDeviceClass       = 0x00,
  .bDeviceSubClass    = 0x00,
  .bDeviceProtocol    = 0x00,
  .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,
  .idVendor           = SWITCH_PRO_VENDOR_ID,   // 0x057E Nintendo
  .idProduct          = SWITCH_PRO_PRODUCT_ID,   // 0x2009 Pro Controller
  .bcdDevice          = 0x0210,
  .iManufacturer      = 0x01,
  .iProduct           = 0x02,
  .iSerialNumber      = 0x03,
  .bNumConfigurations = 0x01
};

uint8_t const* tud_descriptor_device_cb(void) {
  return (uint8_t const*)&desc_device;
}

// ---- HID Report Descriptor (203 bytes) ----
// Matches real Pro Controller / GP2040-CE byte-for-byte
uint8_t const desc_hid_report[] = {
  0x05, 0x01,        // Usage Page (Generic Desktop Ctrls)
  0x15, 0x00,        // Logical Minimum (0)
  0x09, 0x04,        // Usage (Joystick)
  0xA1, 0x01,        // Collection (Application)

  // Report ID 0x30 — Standard input report
  0x85, 0x30,        //   Report ID (48)
  0x05, 0x01,        //   Usage Page (Generic Desktop Ctrls)
  0x05, 0x09,        //   Usage Page (Button)
  0x19, 0x01,        //   Usage Minimum (0x01)
  0x29, 0x0A,        //   Usage Maximum (0x0A)
  0x15, 0x00,        //   Logical Minimum (0)
  0x25, 0x01,        //   Logical Maximum (1)
  0x75, 0x01,        //   Report Size (1)
  0x95, 0x0A,        //   Report Count (10)
  0x55, 0x00,        //   Unit Exponent (0)
  0x65, 0x00,        //   Unit (None)
  0x81, 0x02,        //   Input (Data,Var,Abs)
  0x05, 0x09,        //   Usage Page (Button)
  0x19, 0x0B,        //   Usage Minimum (0x0B)
  0x29, 0x0E,        //   Usage Maximum (0x0E)
  0x15, 0x00,        //   Logical Minimum (0)
  0x25, 0x01,        //   Logical Maximum (1)
  0x75, 0x01,        //   Report Size (1)
  0x95, 0x04,        //   Report Count (4)
  0x81, 0x02,        //   Input (Data,Var,Abs)
  0x75, 0x01,        //   Report Size (1)
  0x95, 0x02,        //   Report Count (2)
  0x81, 0x03,        //   Input (Const,Var,Abs)
  0x0B, 0x01, 0x00, 0x01, 0x00, //   Usage (0x010001)
  0xA1, 0x00,        //   Collection (Physical)
  0x0B, 0x30, 0x00, 0x01, 0x00, //     Usage (X)
  0x0B, 0x31, 0x00, 0x01, 0x00, //     Usage (Y)
  0x0B, 0x32, 0x00, 0x01, 0x00, //     Usage (Z)
  0x0B, 0x35, 0x00, 0x01, 0x00, //     Usage (Rz)
  0x15, 0x00,        //     Logical Minimum (0)
  0x27, 0xFF, 0xFF, 0x00, 0x00, //     Logical Maximum (65534)
  0x75, 0x10,        //     Report Size (16)
  0x95, 0x04,        //     Report Count (4)
  0x81, 0x02,        //     Input (Data,Var,Abs)
  0xC0,              //   End Collection
  0x0B, 0x39, 0x00, 0x01, 0x00, //   Usage (Hat switch)
  0x15, 0x00,        //   Logical Minimum (0)
  0x25, 0x07,        //   Logical Maximum (7)
  0x35, 0x00,        //   Physical Minimum (0)
  0x46, 0x3B, 0x01,  //   Physical Maximum (315)
  0x65, 0x14,        //   Unit (Eng Rot: Degree)
  0x75, 0x04,        //   Report Size (4)
  0x95, 0x01,        //   Report Count (1)
  0x81, 0x02,        //   Input (Data,Var,Abs)
  0x05, 0x09,        //   Usage Page (Button)
  0x19, 0x0F,        //   Usage Minimum (0x0F)
  0x29, 0x12,        //   Usage Maximum (0x12)
  0x15, 0x00,        //   Logical Minimum (0)
  0x25, 0x01,        //   Logical Maximum (1)
  0x75, 0x01,        //   Report Size (1)
  0x95, 0x04,        //   Report Count (4)
  0x81, 0x02,        //   Input (Data,Var,Abs)
  0x75, 0x08,        //   Report Size (8)
  0x95, 0x34,        //   Report Count (52)
  0x81, 0x03,        //   Input (Const,Var,Abs)
  0x06, 0x00, 0xFF,  //   Usage Page (Vendor Defined 0xFF00)

  // Report ID 0x21 — Subcommand reply
  0x85, 0x21,        //   Report ID (33)
  0x09, 0x01,        //   Usage (0x01)
  0x75, 0x08,        //   Report Size (8)
  0x95, 0x3F,        //   Report Count (63)
  0x81, 0x03,        //   Input (Const,Var,Abs)

  // Report ID 0x81 — Config command reply
  0x85, 0x81,        //   Report ID (0x81)
  0x09, 0x02,        //   Usage (0x02)
  0x75, 0x08,        //   Report Size (8)
  0x95, 0x3F,        //   Report Count (63)
  0x81, 0x03,        //   Input (Const,Var,Abs)

  // Report ID 0x01 — UART subcommand (output)
  0x85, 0x01,        //   Report ID (1)
  0x09, 0x03,        //   Usage (0x03)
  0x75, 0x08,        //   Report Size (8)
  0x95, 0x3F,        //   Report Count (63)
  0x91, 0x83,        //   Output (Const,Var,Abs,Volatile)

  // Report ID 0x10 — Rumble (output)
  0x85, 0x10,        //   Report ID (16)
  0x09, 0x04,        //   Usage (0x04)
  0x75, 0x08,        //   Report Size (8)
  0x95, 0x3F,        //   Report Count (63)
  0x91, 0x83,        //   Output (Const,Var,Abs,Volatile)

  // Report ID 0x80 — Config command (output)
  0x85, 0x80,        //   Report ID (0x80)
  0x09, 0x05,        //   Usage (0x05)
  0x75, 0x08,        //   Report Size (8)
  0x95, 0x3F,        //   Report Count (63)
  0x91, 0x83,        //   Output (Const,Var,Abs,Volatile)

  // Report ID 0x82 — Unknown (output)
  0x85, 0x82,        //   Report ID (0x82)
  0x09, 0x06,        //   Usage (0x06)
  0x75, 0x08,        //   Report Size (8)
  0x95, 0x3F,        //   Report Count (63)
  0x91, 0x83,        //   Output (Const,Var,Abs,Volatile)

  0xC0               // End Collection
};

// 203 bytes total
_Static_assert(sizeof(desc_hid_report) == 203, "HID report descriptor must be 203 bytes");

uint8_t const* tud_hid_descriptor_report_cb(uint8_t itf) {
  (void)itf;
  return desc_hid_report;
}

// ---- Configuration Descriptor (raw bytes matching Pro Controller) ----
uint8_t const desc_configuration[] = {
  // Configuration Descriptor (9 bytes)
  0x09,        // bLength
  0x02,        // bDescriptorType (Configuration)
  0x29, 0x00,  // wTotalLength 41
  0x01,        // bNumInterfaces 1
  0x01,        // bConfigurationValue
  0x00,        // iConfiguration (String Index)
  0xA0,        // bmAttributes (Remote Wakeup)
  0xFA,        // bMaxPower 500mA

  // Interface Descriptor (9 bytes)
  0x09,        // bLength
  0x04,        // bDescriptorType (Interface)
  0x00,        // bInterfaceNumber 0
  0x00,        // bAlternateSetting
  0x02,        // bNumEndpoints 2
  0x03,        // bInterfaceClass (HID)
  0x00,        // bInterfaceSubClass
  0x00,        // bInterfaceProtocol
  0x00,        // iInterface (String Index)

  // HID Descriptor (9 bytes)
  0x09,        // bLength
  0x21,        // bDescriptorType (HID)
  0x11, 0x01,  // bcdHID 1.11
  0x00,        // bCountryCode
  0x01,        // bNumDescriptors
  0x22,        // bDescriptorType[0] (HID Report)
  0xCB, 0x00,  // wDescriptorLength[0] 203

  // Endpoint IN Descriptor (7 bytes)
  0x07,        // bLength
  0x05,        // bDescriptorType (Endpoint)
  0x81,        // bEndpointAddress (IN/D2H)
  0x03,        // bmAttributes (Interrupt)
  0x40, 0x00,  // wMaxPacketSize 64
  0x08,        // bInterval 8

  // Endpoint OUT Descriptor (7 bytes)
  0x07,        // bLength
  0x05,        // bDescriptorType (Endpoint)
  0x01,        // bEndpointAddress (OUT/H2D)
  0x03,        // bmAttributes (Interrupt)
  0x40, 0x00,  // wMaxPacketSize 64
  0x08,        // bInterval 8
};

uint8_t const* tud_descriptor_configuration_cb(uint8_t index) {
  (void)index;
  return desc_configuration;
}

// ---- String Descriptors ----
static char const* string_desc_arr[] = {
  (const char[]){0x09, 0x04},  // 0: Language = English
  "Nintendo Co., Ltd.",        // 1: Manufacturer
  "Pro Controller",            // 2: Product
  "000000000001"               // 3: Serial
};

static uint16_t _desc_str[64];

uint16_t const* tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
  (void)langid;
  uint8_t chr_count;
  if (index == 0) {
    memcpy(&_desc_str[1], string_desc_arr[0], 2);
    chr_count = 1;
  } else {
    if (index >= sizeof(string_desc_arr) / sizeof(string_desc_arr[0])) return NULL;
    const char* str = string_desc_arr[index];
    chr_count = (uint8_t)strlen(str);
    if (chr_count > 63) chr_count = 63;
    for (uint8_t i = 0; i < chr_count; i++)
      _desc_str[1 + i] = str[i];
  }
  _desc_str[0] = (uint16_t)((TUSB_DESC_STRING << 8) | (2 * chr_count + 2));
  return _desc_str;
}
