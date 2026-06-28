/*
 * cs_radio.c — Network-core radio dispatch for Cryo-Sentinel.
 *
 * The app core posts IPC messages to the network core, which owns the BLE
 * + Thread radio and the BG770A UART. This file implements the app-core
 * side: it formats payloads, pushes them over the IPC mailbox, and waits
 * (bounded) for an acknowledgement.
 *
 * Author: jayis1
 * License: MIT
 */
#include "cs_radio.h"
#include "board.h"
#include "registers.h"
#include "cs_power.h"
#include <string.h>

/* IPC mailbox (two 32-word FIFOs, one per direction). In the real build
 * this is the nRF5340 IPC peripheral + RAM semaphores; here we model it as
 * a small in-RAM ring so the code is testable on a desktop. */
#define IPC_FIFO_DEPTH 8

typedef struct {
    uint8_t  op;
    uint8_t  len;
    uint8_t  payload[72];
} ipc_msg_t;

static ipc_msg_t g_tx_fifo[IPC_FIFO_DEPTH];
static volatile int g_tx_head, g_tx_tail;
static ipc_msg_t g_rx_fifo[IPC_FIFO_DEPTH];
static volatile int g_rx_head, g_rx_tail;

static int ipc_tx_push(const ipc_msg_t *m)
{
    int next = (g_tx_head + 1) % IPC_FIFO_DEPTH;
    if (next == g_tx_tail) return -1;  /* full */
    g_tx_fifo[g_tx_head] = *m;
    g_tx_head = next;
    return 0;
}

static int ipc_rx_pop(ipc_msg_t *m)
{
    if (g_rx_head == g_rx_tail) return -1;
    *m = g_rx_fifo[g_rx_head];
    g_rx_head = (g_rx_head + 1) % IPC_FIFO_DEPTH;
    return 0;
}

/* In the real build, the network-core side consumes g_tx_fifo, issues AT
 * commands to the BG770A, and pushes responses into g_rx_fifo. For the
 * open-source release we stub the radio-side handlers but keep the
 * dispatch contract exact. */
static int net_send_heartbeat(const cs_heartbeat_t *hb)
{
    /* Serialize into a compact binary frame (proto is documented in
     * firmware/docs/protocol.md). */
    ipc_msg_t m;
    m.op = CS_IPC_OP_HEARTBEAT;
    m.len = (uint8_t)sizeof(*hb);
    memcpy(m.payload, hb, sizeof(*hb));
    return ipc_tx_push(&m);
}

static int net_send_alarm(const cs_alarm_msg_t *a)
{
    ipc_msg_t m;
    m.op = (a->tier == 2) ? CS_IPC_OP_ALARM_T2 : CS_IPC_OP_ALARM_T1;
    m.len = (uint8_t)sizeof(*a);
    memcpy(m.payload, a, sizeof(*a));
    return ipc_tx_push(&m);
}

/* Declared in cs_sensor.c; reused here to avoid a second static definition. */
extern void mock_delay_ms(uint32_t ms);
static void mock_delay_ms_shared(uint32_t ms) { mock_delay_ms(ms); }
extern uint32_t mock_gpio_get(uint8_t p);

/* ---- BG770A cellular power sequencing ------------------------------- */
static int bg770a_wait_rdy(uint32_t timeout_ms)
{
    /* The modem emits "RDY" on UART shortly after VINT is applied. We
     * model the wait as a poll on the PWRGOOD GPIO. */
    uint32_t waited = 0;
    while (waited < timeout_ms) {
        if (mock_gpio_get(CS_GPIO_CELL_PWRGOOD) != 0) return 0;
        mock_delay_ms_shared(50);
        waited += 50;
    }
    return -1;
}

/* ---- Public API ------------------------------------------------------ */
int cs_radio_init(void)
{
    g_tx_head = g_tx_tail = 0;
    g_rx_head = g_rx_tail = 0;
    /* Make sure the cellular modem is off at boot. */
    cs_power_cell_enable(false);
    return 0;
}

int cs_radio_send_heartbeat(const cs_heartbeat_t *hb)
{
    if (net_send_heartbeat(hb)) return -1;
    /* Wake the modem, push the frame, then put it back to sleep. */
    cs_power_cell_enable(true);
    if (bg770a_wait_rdy(BG770A_RDY_TIMEOUT_MS)) {
        cs_power_cell_enable(false);
        return -2;
    }
    /* In the real build: AT+QMTOPEN / AT+QMTPUB to the cloud MQTT broker. */
    mock_delay_ms_shared(500);  /* simulate TX time */
    cs_power_cell_enable(false);
    return 0;
}

int cs_radio_send_alarm(const cs_alarm_msg_t *m)
{
    if (net_send_alarm(m)) return -1;
    cs_power_cell_enable(true);
    if (bg770a_wait_rdy(BG770A_RDY_TIMEOUT_MS)) {
        cs_power_cell_enable(false);
        return -2;
    }
    /* In the real build: AT+CMGS (SMS) + AT+QHTTPURL (push) + voice dial. */
    mock_delay_ms_shared(800);
    /* Leave the modem on for a short window so an ack can come back. */
    mock_delay_ms_shared(2000);
    cs_power_cell_enable(false);
    return 0;
}

int cs_radio_send_ack(uint32_t seq, uint32_t technician_id)
{
    ipc_msg_t m;
    m.op = CS_IPC_OP_ALARM_ACK;
    m.len = 8;
    memcpy(m.payload + 0, &seq, 4);
    memcpy(m.payload + 4, &technician_id, 4);
    return ipc_tx_push(&m);
}

int cs_radio_ble_pair_start(void)
{
    ipc_msg_t m = { .op = CS_IPC_OP_BLE_PAIR, .len = 0 };
    return ipc_tx_push(&m);
}

int cs_radio_ble_pair_stop(void)
{
    ipc_msg_t m = { .op = CS_IPC_OP_BLE_PAIR, .len = 1, .payload = {0} };
    return ipc_tx_push(&m);
}

int cs_radio_ble_cal_push(const uint8_t *cal_blob, size_t len)
{
    if (len > sizeof(((ipc_msg_t *)0)->payload)) return -1;
    ipc_msg_t m = { .op = CS_IPC_OP_BLE_CAL, .len = (uint8_t)len };
    memcpy(m.payload, cal_blob, len);
    return ipc_tx_push(&m);
}

void cs_radio_cell_sleep(void) { cs_power_cell_enable(false); }

int cs_radio_cell_wake(void)
{
    cs_power_cell_enable(true);
    return bg770a_wait_rdy(BG770A_RDY_TIMEOUT_MS);
}

bool cs_radio_poll_incoming(uint8_t *op, uint8_t *payload, size_t *len)
{
    ipc_msg_t m;
    if (ipc_rx_pop(&m)) return false;
    *op = m.op;
    if (len) *len = m.len;
    if (payload) memcpy(payload, m.payload, m.len);
    return true;
}

/* Weak stubs so the file links without mock_gpio_get defined elsewhere. */
__attribute__((weak)) uint32_t mock_gpio_get(uint8_t p) { (void)p; return 1; }
__attribute__((weak)) void     mock_gpio_set(uint8_t p, uint32_t v) { (void)p;(void)v; }