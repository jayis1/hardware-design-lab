/*
 * ble_telemetry.c — HydroFluor BLE 5 (BM71) GATT telemetry server
 * Author: jayis1
 * License: MIT
 *
 * The BM71 module is driven over USART1 using its HCI UART transport.
 * A minimal GATT server is built at boot advertising the HydroFluor service
 * (0xFF00) with five characteristics (Control, DataStream, Calibrate,
 * DeviceInfo). The firmware implements a small command parser and pushes
 * sample-record notifications to connected centrals.
 *
 * The BM71 module handles the full BLE stack; we exchange HCI commands
 * (vendor-specific) to configure the GATT table and send notifications.
 * A full BM71 driver is large; this file implements the command surface
 * that main.c needs (init, notify, poll) using a simplified serial frame.
 */
#include "ble_telemetry.h"
#include "../registers.h"
#include <string.h>

static uint8_t g_connected = 0;
static uint16_t g_seq = 0;
static char g_fw[16]   = "0.1.0";
static char g_serial[16] = "HF-0000";
static uint32_t g_cal_date = 0;

/* RX ring buffer (UART interrupt-driven) */
#define BLE_RX_BUF 128
static volatile uint8_t  g_rxbuf[BLE_RX_BUF];
static volatile uint16_t g_rx_head = 0, g_rx_tail = 0;

/* Pending command bitmask set by the parser */
static volatile uint16_t g_pending_cmds = 0;
static volatile uint16_t g_period_param  = 0;

void ble_init(void)
{
    /* Enable USART1 (APB2ENR bit 14) + GPIOA */
    RCC->APB2ENR |= (1U << 14);
    RCC->AHB2ENR |= (1U << 0);

    /* PA9 TX, PA10 RX = AF7 (USART1) — note these are shared with TIM1 CH2/3
     * alternate; in the final PCB they're routed to dedicated pins. For
     * the firmware bring-up we use PA9/PA10. */
    gpio_mode(GPIOA, 9, GPIO_MODE_AF);  gpio_af(GPIOA, 9, 7);
    gpio_mode(GPIOA, 10, GPIO_MODE_AF); gpio_af(GPIOA, 10, 7);

    /* BM71 reset pin PA0 (output, active low) */
    gpio_mode(BLE_RST_PORT, BLE_RST_PIN, GPIO_MODE_OUTPUT);
    gpio_clr(BLE_RST_PORT, BLE_RST_PIN);
    for (volatile int i = 0; i < 10000; ++i) { }
    gpio_set(BLE_RST_PORT, BLE_RST_PIN);

    /* USART1: 115200 8N1, enable RX interrupt */
    USART1->BRR = BOARD_PCLK2_HZ / BLE_UART_BAUD;
    USART1->CR1 = USART_CR1_UE | USART_CR1_RE | USART_CR1_TE | USART_CR1_RXNEIE;
    /* (The EXTI/NVIC enable for USART1 IRQ is set up here) */
    nvic_enable(37);   /* USART1_IRQn = 37 on L4R9 */

    /* Send BM71 initialization HCI sequence (vendor command to build GATT).
     * In the production driver these are real HCI packets; here we send a
     * short marker so the host can log boot. */
    const char *boot = "BM71 INIT\n";
    for (const char *p = boot; *p; ++p) uart_putc(USART1, (uint8_t)*p);
}

/* USART1 interrupt handler (called from the startup vector table) */
void USART1_IRQHandler(void)
{
    if (USART1->ISR & USART_ISR_RXNE) {
        uint8_t b = (uint8_t)USART1->RDR;
        uint16_t next = (g_rx_head + 1) & (BLE_RX_BUF - 1);
        if (next != g_rx_tail) {
            g_rxbuf[g_rx_head] = b;
            g_rx_head = next;
        }
    }
}

static int rx_avail(void) { return g_rx_head != g_rx_tail; }
static uint8_t rx_get(void) {
    uint8_t b = g_rxbuf[g_rx_tail];
    g_rx_tail = (g_rx_tail + 1) & (BLE_RX_BUF - 1);
    return b;
}

/* Parse a simple text command line from the BLE link.
 * Recognized commands:
 *   START              -> BLE_CMD_START
 *   STOP               -> BLE_CMD_STOP
 *   PUMP ON            -> BLE_CMD_PUMP_ON
 *   PUMP OFF           -> BLE_CMD_PUMP_OFF
 *   CAL <analyte> <ref>-> BLE_CMD_CALIBRATE
 *   PERIOD <ms>        -> BLE_CMD_SET_PERIOD
 *   CONNECT            -> (internal) sets g_connected
 *   DISCONNECT         -> clears g_connected
 */
static void parse_line(const char *line)
{
    if (!strncmp(line, "CONNECT", 7))      { g_connected = 1; return; }
    if (!strncmp(line, "DISCONNECT", 10))  { g_connected = 0; return; }
    if (!strncmp(line, "START", 5))         { g_pending_cmds |= BLE_CMD_START; return; }
    if (!strncmp(line, "STOP", 4))          { g_pending_cmds |= BLE_CMD_STOP;  return; }
    if (!strncmp(line, "PUMP ON", 7))       { g_pending_cmds |= BLE_CMD_PUMP_ON;  return; }
    if (!strncmp(line, "PUMP OFF", 8))      { g_pending_cmds |= BLE_CMD_PUMP_OFF; return; }
    if (!strncmp(line, "CAL", 3))          { g_pending_cmds |= BLE_CMD_CALIBRATE; return; }
    if (!strncmp(line, "PERIOD", 6)) {
        int ms = 0;
        const char *p = line + 7;
        while (*p >= '0' && *p <= '9') { ms = ms*10 + (*p - '0'); ++p; }
        if (ms >= (int)SURVEY_MIN_PERIOD_MS && ms <= (int)SURVEY_MAX_PERIOD_MS) {
            g_period_param = (uint16_t)ms;
            g_pending_cmds |= BLE_CMD_SET_PERIOD;
        }
    }
}

uint16_t ble_poll(uint16_t *param_period_ms_out)
{
    static char line[64];
    static uint8_t li = 0;

    while (rx_avail()) {
        uint8_t b = rx_get();
        if (b == '\n' || b == '\r') {
            if (li > 0) {
                line[li] = '\0';
                parse_line(line);
                li = 0;
            }
        } else if (li < sizeof(line) - 1) {
            line[li++] = (char)b;
        }
    }
    uint16_t c = g_pending_cmds;
    if ((c & BLE_CMD_SET_PERIOD) && param_period_ms_out) {
        *param_period_ms_out = g_period_param;
    }
    g_pending_cmds = 0;
    return c;
}

uint8_t ble_connected(void) { return g_connected; }

/* Pack a sample_record_t into the 24-byte binary format from the BLE spec
 * and send it as a notification over the DataStream characteristic. */
void ble_notify_sample(const sample_record_t *rec)
{
    if (!g_connected) return;
    uint8_t pkt[24];
    pkt[0]  = (uint8_t)(rec->seq & 0xFF);
    pkt[1]  = (uint8_t)(rec->seq >> 8);
    pkt[2]  = (uint8_t)(rec->timestamp & 0xFF);
    pkt[3]  = (uint8_t)((rec->timestamp >> 8)  & 0xFF);
    pkt[4]  = (uint8_t)((rec->timestamp >> 16) & 0xFF);
    pkt[5]  = (uint8_t)((rec->timestamp >> 24) & 0xFF);
    pkt[6]  = (uint8_t)(rec->depth_cm & 0xFF);
    pkt[7]  = (uint8_t)(rec->depth_cm >> 8);
    pkt[8]  = (uint8_t)(rec->temp_c01 & 0xFF);
    pkt[9]  = (uint8_t)(rec->temp_c01 >> 8);
    pkt[10] = (uint8_t)(rec->battery_mv & 0xFF);
    pkt[11] = (uint8_t)(rec->battery_mv >> 8);
    pkt[12] = (uint8_t)(rec->cdom_ppb & 0xFF);
    pkt[13] = (uint8_t)(rec->cdom_ppb >> 8);
    pkt[14] = (uint8_t)(rec->chla_ugl & 0xFF);
    pkt[15] = (uint8_t)(rec->chla_ugl >> 8);
    pkt[16] = (uint8_t)(rec->phyc_ugl & 0xFF);
    pkt[17] = (uint8_t)(rec->phyc_ugl >> 8);
    pkt[18] = (uint8_t)(rec->oil_ppb & 0xFF);
    pkt[19] = (uint8_t)(rec->oil_ppb >> 8);
    pkt[20] = (uint8_t)(rec->turb_ntu_x100 & 0xFF);
    pkt[21] = (uint8_t)(rec->turb_ntu_x100 >> 8);
    pkt[22] = (uint8_t)(rec->flags & 0xFF);
    pkt[23] = (uint8_t)(rec->flags >> 8);

    /* Send as a NOTIFY HCI frame to the BM71. We write a short header
     * followed by the 24-byte payload; the module wraps it in a GATT
     * notification on characteristic 0xFF11. */
    uart_putc(USART1, 0x02);   /* NOTIFY opcode */
    uart_putc(USART1, 24);
    for (int i = 0; i < 24; ++i) uart_putc(USART1, pkt[i]);
    g_seq = rec->seq;
}

void ble_notify_calib(uint8_t analyte, int16_t fit_residual)
{
    if (!g_connected) return;
    uart_putc(USART1, 0x03);   /* CALIB result opcode */
    uart_putc(USART1, analyte);
    uart_putc(USART1, (uint8_t)(fit_residual & 0xFF));
    uart_putc(USART1, (uint8_t)((fit_residual >> 8) & 0xFF));
}

void ble_set_device_info(const char *fw, const char *serial, uint32_t cal_date)
{
    strncpy(g_fw, fw, sizeof(g_fw) - 1);
    strncpy(g_serial, serial, sizeof(g_serial) - 1);
    g_cal_date = cal_date;
}