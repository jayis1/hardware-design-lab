/*
 * wifi.c — ATWINC1500 Wi-Fi driver (SPI3)
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * License: MIT
 *
 * The ATWINC1500 is an Atmel/Microchip Wi-Fi network controller with a
 * SPI host interface and its own firmware. In the production build this
 * file links against the vendor's m2m_wifi / m2m_socket library. Here we
 * provide the application-facing interface and a thin shim that
 * captures the call sequence for review and host-side testing.
 */
#include "wifi.h"
#include "i2c.h"
#include "registers.h"
#include "board.h"
#include <string.h>

static wifi_mode_t s_mode = WIFI_MODE_IDLE;
static int s_connected = 0;
static int s_client_connected = 0;
static int32_t s_last_rssi = -90;

/* Vendor API hooks — provided by the linked m2m_wifi library. */
extern int m2m_wifi_init(const char *firmware_path);
extern int m2m_wifi_connect(const char *ssid, const char *pass);
extern int m2m_wifi_disconnect(void);
extern int m2m_wifi_get_rssi(int32_t *rssi);
extern int m2m_socket_listen(uint16_t port);
extern int m2m_socket_accept(void);
extern int m2m_socket_recv(uint8_t *buf, uint32_t max, uint32_t timeout_ms);
extern int m2m_socket_send(const uint8_t *buf, uint32_t len);
extern int m2m_socket_close(void);

muon_status_t wifi_init(void)
{
    /* Configure SPI3 pins (PC10-12 + PD4 CS) for the ATWINC1500 */
    RCC_APB2ENR |= RCC_APB2ENR_SPI3EN;
    /* (full pin mux omitted for brevity — same pattern as fpga.c) */

    /* IRQ pin input, CS output, RESET output (high = run) */
    GPIO_MODER(WIFI_IRQ_PORT) &= ~(3U << (WIFI_IRQ_PIN * 2));
    GPIO_PUPDR(WIFI_IRQ_PORT) |= (1U << (WIFI_IRQ_PIN * 2));
    GPIO_MODER(WIFI_CS_PORT)  |= (1U << (WIFI_CS_PIN * 2));
    GPIO_BSRR(WIFI_CS_PORT)   = WIFI_CS_PIN;
    GPIO_MODER(WIFI_RESET_PORT) |= (1U << (WIFI_RESET_PIN * 2));
    GPIO_BSRR(WIFI_RESET_PORT)  = WIFI_RESET_PIN;  /* deassert reset */

    if (m2m_wifi_init(NULL) != 0) return MUON_ERR_WIFI;
    s_mode = WIFI_MODE_IDLE;
    return MUON_OK;
}

muon_status_t wifi_connect(const char *ssid, const char *pass)
{
    if (!ssid) return MUON_ERR_INVALID_ARG;
    if (m2m_wifi_connect(ssid, pass ? pass : "") != 0) return MUON_ERR_WIFI;
    s_mode = WIFI_MODE_STA;
    s_connected = 1;
    return MUON_OK;
}

muon_status_t wifi_start_ap(const char *ssid, const char *pass)
{
    (void)pass;
    if (!ssid) return MUON_ERR_INVALID_ARG;
    /* m2m_wifi_start_ap(ssid, pass, 1); — vendor API call */
    s_mode = WIFI_MODE_AP;
    s_connected = 1;
    return MUON_OK;
}

muon_status_t wifi_disconnect(void)
{
    m2m_wifi_disconnect();
    s_connected = 0;
    s_mode = WIFI_MODE_IDLE;
    return MUON_OK;
}

int wifi_is_connected(void) { return s_connected; }
int32_t wifi_rssi_dbm(void)
{
    int32_t r = 0;
    if (m2m_wifi_get_rssi(&r) == 0) s_last_rssi = r;
    return s_last_rssi;
}

muon_status_t wifi_tcp_listen(void)
{
    if (m2m_socket_listen(WIFI_PORT) != 0) return MUON_ERR_WIFI;
    return MUON_OK;
}

int wifi_tcp_client_connected(void) { return s_client_connected; }

int32_t wifi_tcp_recv(uint8_t *buf, uint32_t max, uint32_t timeout_ms)
{
    if (!buf) return -1;
    int32_t n = m2m_socket_recv(buf, max, timeout_ms);
    if (n > 0) s_client_connected = 1;
    return n;
}

int32_t wifi_tcp_send(const uint8_t *buf, uint32_t len)
{
    if (!buf || len == 0) return -1;
    return m2m_socket_send(buf, len);
}

void wifi_tcp_close(void)
{
    m2m_socket_close();
    s_client_connected = 0;
}

/* ---- Vendor API weak stubs (so the code links for review) ---- */
__attribute__((weak)) int m2m_wifi_init(const char *p) { (void)p; return 0; }
__attribute__((weak)) int m2m_wifi_connect(const char *s, const char *p)
{ (void)s; (void)p; return 0; }
__attribute__((weak)) int m2m_wifi_disconnect(void) { return 0; }
__attribute__((weak)) int m2m_wifi_get_rssi(int32_t *r) { if (r) *r = -70; return 0; }
__attribute__((weak)) int m2m_socket_listen(uint16_t p) { (void)p; return 0; }
__attribute__((weak)) int m2m_socket_accept(void) { return 0; }
__attribute__((weak)) int m2m_socket_recv(uint8_t *b, uint32_t m, uint32_t t)
{ (void)b; (void)m; (void)t; return 0; }
__attribute__((weak)) int m2m_socket_send(const uint8_t *b, uint32_t l)
{ (void)b; return (int)l; }
__attribute__((weak)) int m2m_socket_close(void) { return 0; }