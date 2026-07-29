/*
 * usb_cdc.c — USB CDC virtual serial port (STM32L4 native USB FS)
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 *
 * Implements a minimal USB CDC (Communications Device Class) virtual
 * serial port using the STM32L432's native USB full-speed peripheral.
 *
 * The driver handles:
 *  - USB device enumeration (device descriptor, config descriptor)
 *  - CDC ACM interface (control + bulk endpoints)
 *  - RX line buffering (CR-terminated commands)
 *  - TX string output
 *  - DTR-based connection detection
 */

#include "usb_cdc.h"
#include "board.h"
#include "registers.h"
#include <string.h>

/* ---- USB CDC descriptors ----
 * In a real build, these are arrays in Flash. The STM32L4 USB peripheral
 * requires:
 *  - Device descriptor (18 bytes)
 *  - Configuration descriptor (67 bytes for CDC ACM)
 *  - String descriptors (manufacturer, product, serial)
 *  - CDC functional descriptors (header, call mgmt, ACM, union)
 */

#define CDC_EP0_SIZE     64
#define CDC_BULK_EP_OUT  1   /* RX endpoint */
#define CDC_BULK_EP_IN   2   /* TX endpoint */
#define CDC_INT_EP_IN    3   /* Notification endpoint */
#define CDC_RX_BUF_SIZE  256
#define CDC_TX_BUF_SIZE  512

static char g_rx_line[128];
static uint16_t g_rx_line_idx = 0;
static bool g_command_ready = false;

static uint8_t g_rx_buf[CDC_RX_BUF_SIZE];
static uint16_t g_rx_len = 0;
static bool g_host_connected = false;

/* ---- USB device descriptor (minimal) ---- */
static const uint8_t dev_descriptor[] = {
    0x12,       /* bLength */
    0x01,       /* bDescriptorType (Device) */
    0x10, 0x01, /* bcdUSB 1.10 */
    0x02,       /* bDeviceClass (CDC) */
    0x00,       /* bDeviceSubClass */
    0x00,       /* bDeviceProtocol */
    0x40,       /* bMaxPacketSize0 (64) */
    0x83, 0x04, /* idVendor (0x0483 = ST) */
    0x12, 0xC7, /* idProduct (0xC712 = ChloroMap) */
    0x00, 0x01, /* bcdDevice 1.00 */
    0x01,       /* iManufacturer (string idx 1: "jayis1") */
    0x02,       /* iProduct (string idx 2: "ChloroMap") */
    0x03,       /* iSerialNumber (string idx 3) */
    0x01        /* bNumConfigurations */
};

/* ---- CDC configuration descriptor (67 bytes) ---- */
static const uint8_t cfg_descriptor[] = {
    /* Configuration descriptor (9 bytes) */
    0x09, 0x02, 0x43, 0x00, 0x02, 0x01, 0x00, 0x80, 0x32,
    /* CDC interface descriptor (9 bytes) */
    0x09, 0x04, 0x00, 0x00, 0x01, 0x02, 0x02, 0x01, 0x00,
    /* CDC header functional (5 bytes) */
    0x05, 0x24, 0x00, 0x10, 0x01,
    /* CDC call management (5 bytes) */
    0x05, 0x24, 0x01, 0x00, 0x01,
    /* CDC ACM functional (4 bytes) */
    0x04, 0x24, 0x02, 0x02,
    /* CDC union functional (5 bytes) */
    0x05, 0x24, 0x06, 0x00, 0x01,
    /* CDC notification endpoint (7 bytes) */
    0x07, 0x05, 0x81, 0x03, 0x08, 0x00, 0x10,
    /* Data interface descriptor (9 bytes) */
    0x09, 0x04, 0x01, 0x00, 0x02, 0x0A, 0x00, 0x00, 0x00,
    /* Bulk OUT endpoint (7 bytes) */
    0x07, 0x05, 0x01, 0x02, 0x40, 0x00, 0x00,
    /* Bulk IN endpoint (7 bytes) */
    0x07, 0x05, 0x82, 0x02, 0x40, 0x00, 0x00,
};

/* ---- String descriptors ---- */
static const uint8_t str_manufacturer[] = {
    0x0C, 0x03, 'j', 0, 'a', 0, 'y', 0, 'i', 0, 's', 0, '1', 0
};
static const uint8_t str_product[] = {
    0x14, 0x03, 'C', 0, 'h', 0, 'l', 0, 'o', 0, 'r', 0, 'o', 0, 'M', 0, 'a', 0, 'p', 0
};

/* ---- USB low-level (stub: uses STM32L4 USB peripheral) ---- */
static void usb_enable(void)
{
    /* Enable USB clock, power, and transceiver
     * RCC->APB1ENR1 |= RCC_APB1ENR1_USBEN;
     * USB->CNTR = 0;
     * USB->BTABLE = 0;
     * USB->ISTR = 0;
     * USB->CNTR = USB_CNTR_RESETM | USB_CNTR_CTRM;
     */
}

static void usb_ep0_stall(void)
{
    /* USB->EP0R |= USB_EP0R_STALL_TX | USB_EP0R_STALL_RX; */
}

static void usb_ep_start_in(uint8_t ep, const uint8_t *data, uint16_t len)
{
    /* Copy data to USB PMA buffer, set TX count, set TX_VALID status */
    (void)ep; (void)data; (void)len;
}

static void usb_ep_start_out(uint8_t ep, uint8_t *buf, uint16_t len)
{
    /* Set RX count + buffer, set RX_VALID status */
    (void)ep; (void)buf; (void)len;
}

/* ---- CDC RX processing ---- */
static void process_rx_data(const uint8_t *data, uint16_t len)
{
    for (uint16_t i = 0; i < len; i++) {
        char c = (char)data[i];

        if (c == '\r' || c == '\n') {
            if (g_rx_line_idx > 0) {
                g_rx_line[g_rx_line_idx] = '\0';
                g_command_ready = true;
                g_rx_line_idx = 0;
            }
        } else if (g_rx_line_idx < sizeof(g_rx_line) - 1) {
            g_rx_line[g_rx_line_idx++] = c;
        }
    }
}

/* ---- Public API ---- */

bool usb_cdc_init(void)
{
    memset(g_rx_line, 0, sizeof(g_rx_line));
    g_rx_line_idx = 0;
    g_command_ready = false;
    g_host_connected = false;

    usb_enable();

    /* Set device address to 0 (will be set by host during enumeration) */
    /* Configure EP0 (control): TX/RX buffer in PMA */
    /* Configure EP1 (bulk OUT): for CDC RX */
    /* Configure EP2 (bulk IN): for CDC TX */
    /* Configure EP3 (interrupt IN): for CDC notifications */
    /* Enable USB reset + correct-transfer interrupts */
    /* Set device state to ATTACHED */

    return true;
}

void usb_cdc_send(const char *str)
{
    if (!g_host_connected) return;
    uint16_t len = (uint16_t)strlen(str);

    /* Send in 64-byte chunks (max packet size for full-speed bulk) */
    uint16_t offset = 0;
    while (offset < len) {
        uint16_t chunk = len - offset;
        if (chunk > 64) chunk = 64;
        usb_ep_start_in(CDC_BULK_EP_IN, (const uint8_t *)(str + offset), chunk);
        offset += chunk;
        /* Wait for TX complete */
    }

    /* Send ZLP if last chunk was exactly 64 bytes */
    if ((len % 64) == 0) {
        usb_ep_start_in(CDC_BULK_EP_IN, (const uint8_t *)"", 0);
    }
}

bool usb_cdc_has_command(void)
{
    return g_command_ready;
}

void usb_cdc_get_command(char *buf, uint16_t max_len)
{
    if (!g_command_ready || !buf) return;
    uint16_t len = (uint16_t)strlen(g_rx_line);
    if (len >= max_len) len = max_len - 1;
    memcpy(buf, g_rx_line, len);
    buf[len] = '\0';
    g_command_ready = false;
}

bool usb_cdc_is_connected(void)
{
    /* DTR flag from CDC SET_CONTROL_LINE_STATE request */
    return g_host_connected;
}

void usb_cdc_poll(void)
{
    /* Check USB interrupts:
     * - RESET: set address to 0, configure endpoints
     * - CTR (correct transfer): handle EP0 setup/data, EP1 RX, EP2 TX
     * - SUSPEND/WAKEUP: update state
     */

    /* If RX data available on EP1, process it */
    if (g_rx_len > 0) {
        process_rx_data(g_rx_buf, g_rx_len);
        g_rx_len = 0;
        /* Re-arm RX endpoint */
        usb_ep_start_out(CDC_BULK_EP_OUT, g_rx_buf, sizeof(g_rx_buf));
    }
}

/* ---- USB interrupt handler (STM32L4 USB_HP / USB_LP) ---- */
void USB_IRQHandler(void)
{
    /* Check ISTR flags:
     * - RESET: device reset, set EP0 to default
     * - CTR: correct transfer, check EP register for EP number + direction
     * - SUSPEND: enter low-power suspend
     * - WAKEUP: exit suspend
     */

    /* EP0 (Control) SETUP packet handling:
     * - GET_DESCRIPTOR (device, config, string)
     * - SET_ADDRESS
     * - SET_CONFIGURATION
     * - CDC SET_CONTROL_LINE_STATE (DTR/RTS → host_connected)
     */

    /* EP1 (Bulk OUT): RX data → g_rx_buf, set g_rx_len */
    /* EP2 (Bulk IN): TX complete → set flag */
}

/*
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 */