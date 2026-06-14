/*
 * SkyPilot — u-blox LARA-R6 LTE Modem Driver Implementation
 * UART8 with DMA ring buffer on STM32H743VIT6
 *
 * Handles modem power sequencing, AT command interface,
 * network registration, PDP context, and MQTT pub/sub
 */

#include "lte_lara_r6.h"
#include "../registers.h"
#include "../board.h"
#include <string.h>
#include <stdlib.h>

/* ==================== Private Variables ==================== */

static lte_state_t lte_state = LTE_STATE_OFF;
static volatile uint16_t ring_head = 0;
static volatile uint16_t ring_tail = 0;
static uint8_t ring_buf[LTE_RING_BUF_SIZE];
static char at_rsp_buf[LTE_RSP_BUF_SIZE];
static volatile uint16_t at_rsp_len = 0;
static volatile uint8_t at_rsp_ready = 0;

/* ==================== UART/DMA Helpers ==================== */

static void uart8_putc(char c)
{
    while (!(USART8->ISR & USART_ISR_TXE))
        ;
    USART8->TDR = (uint32_t)c;
}

static void uart8_puts(const char *s)
{
    while (*s) {
        uart8_putc(*s++);
    }
}

static void uart8_write_dma(const uint8_t *data, uint16_t len)
{
    /* Configure DMA2 Stream6 for USART8_TX (Channel 5) */
    DMA2_Stream6->CR &= ~DMA_SxCR_EN;
    while (DMA2_Stream6->CR & DMA_SxCR_EN)
        ;

    DMA2_Stream6->PAR   = (uint32_t)&USART8->TDR;
    DMA2_Stream6->M0AR  = (uint32_t)data;
    DMA2_Stream6->NDTR  = len;
    DMA2_Stream6->CR    = (5UL << DMA_SxCR_CHSEL_Pos) | /* Channel 5 */
                           DMA_SxCR_MINC |
                           DMA_SxCR_DIR_0 |    /* Memory-to-peripheral */
                           DMA_SxCR_TCIE |
                           DMA_SxCR_EN;

    USART8->CR3 |= USART_CR3_DMAT;
}

/* ==================== Ring Buffer ==================== */

static uint16_t ring_available(void)
{
    uint16_t h = ring_head;
    uint16_t t = ring_tail;
    return (h >= t) ? (h - t) : (LTE_RING_BUF_SIZE - t + h);
}

static uint8_t ring_read(void)
{
    uint8_t c = ring_buf[ring_tail];
    ring_tail = (ring_tail + 1) % LTE_RING_BUF_SIZE;
    return c;
}

/* ==================== IRQ Handlers ==================== */

void USART8_IRQHandler(void)
{
    if (USART8->ISR & USART_ISR_RXNE) {
        uint8_t c = (uint8_t)(USART8->RDR & 0xFF);
        uint16_t next = (ring_head + 1) % LTE_RING_BUF_SIZE;
        if (next != ring_tail) {
            ring_buf[ring_head] = c;
            ring_head = next;
        }

        /* AT response detection: look for "\r\n" terminators */
        if (c == '\n' && at_rsp_len > 0 && at_rsp_buf[at_rsp_len - 1] == '\r') {
            at_rsp_buf[at_rsp_len] = '\0';
            at_rsp_ready = 1;
        } else if (c != '\r') {
            if (at_rsp_len < LTE_RSP_BUF_SIZE - 1) {
                at_rsp_buf[at_rsp_len++] = c;
            }
        }
    }

    /* Clear overrun flag if set */
    if (USART8->ISR & USART_ISR_ORE) {
        USART8->ICR = USART_ICR_ORECF;
    }
}

void DMA2_Stream6_IRQHandler(void)
{
    if (DMA2->HISR & DMA_HISR_TCIF6) {
        DMA2->HIFCR = DMA_HIFCR_CTCIF6;
        USART8->CR3 &= ~USART_CR3_DMAT;
    }
}

/* ==================== Power Control ==================== */

int lte_init(void)
{
    /* Configure UART8 GPIOs */
    /* PC10 = TX (AF11), PC9 = RX (AF11) — already done in board_init() */

    /* Configure LTE_PWR_ON (PA0) as output push-pull */
    GPIOA->MODER  &= ~(3UL << (LTE_PWR_ON_PIN * 2));
    GPIOA->MODER  |=  (1UL << (LTE_PWR_ON_PIN * 2));  /* Output */
    GPIOA->OTYPER &= ~(1UL << LTE_PWR_ON_PIN);          /* Push-pull */
    GPIOA->OSPEEDR |= (3UL << (LTE_PWR_ON_PIN * 2));   /* High speed */
    GPIOA->PUPDR   &= ~(3UL << (LTE_PWR_ON_PIN * 2));  /* No pull */

    /* Configure LTE_STATUS (PD9) as input */
    GPIOD->MODER &= ~(3UL << (LTE_STATUS_PIN * 2));  /* Input */
    GPIOD->PUPDR |= (1UL << (LTE_STATUS_PIN * 2));   /* Pull-up */

    /* Initialize ring buffer */
    ring_head = 0;
    ring_tail = 0;
    at_rsp_len = 0;
    at_rsp_ready = 0;
    lte_state = LTE_STATE_OFF;

    return 0;
}

int lte_power_on(void)
{
    lte_state = LTE_STATE_POWERING_ON;

    /* LARA-R6 power-on sequence:
     * 1. Assert PWR_ON low for at least 30ms
     * 2. Wait for STATUS pin to go high (up to 5s)
     * 3. Wait for UART ready (~2s after STATUS goes high)
     */

    LTE_PWR_ON_LOW();

    /* Wait 50ms (PWR_ON pulse width) */
    for (volatile uint32_t i = 0; i < 5000000; i++)
        ;

    LTE_PWR_ON_HIGH();

    /* Wait for STATUS high */
    uint32_t timeout = 5000;  /* 5 second timeout */
    while (!LTE_STATUS_READ() && timeout--) {
        for (volatile uint32_t i = 0; i < 1000; i++)
            ;
    }

    if (!LTE_STATUS_READ()) {
        lte_state = LTE_STATE_ERROR;
        return -1;
    }

    /* Wait 2s for UART to be ready */
    for (volatile uint32_t i = 0; i < 200000000; i++)
        ;

    /* Enable USART8 RX interrupt */
    USART8->CR1 |= USART_CR1_RXNEIE;
    NVIC_EnableIRQ(USART8_IRQn);

    lte_state = LTE_STATE_INITIALIZING;

    /* Send AT to verify modem responds */
    int ret = lte_send_at("AT", at_rsp_buf, LTE_RSP_BUF_SIZE, 3000);
    if (ret != 0) {
        lte_state = LTE_STATE_ERROR;
        return -2;
    }

    return 0;
}

int lte_power_off(void)
{
    /* AT+CFUN=0 — minimum functionality */
    lte_send_at("AT+CFUN=0", at_rsp_buf, LTE_RSP_BUF_SIZE, 5000);

    /* PWR_ON pulse to power down */
    LTE_PWR_ON_LOW();
    for (volatile uint32_t i = 0; i < 5000000; i++)
        ;
    LTE_PWR_ON_HIGH();

    /* Wait for STATUS to go low */
    uint32_t timeout = 10000;
    while (LTE_STATUS_READ() && timeout--) {
        for (volatile uint32_t i = 0; i < 1000; i++)
            ;
    }

    USART8->CR1 &= ~USART_CR1_RXNEIE;
    NVIC_DisableIRQ(USART8_IRQn);

    lte_state = LTE_STATE_OFF;
    return 0;
}

int lte_reset(void)
{
    /* AT+CFUN=1,1 — Reset module */
    lte_send_at("AT+CFUN=1,1", at_rsp_buf, LTE_RSP_BUF_SIZE, 10000);

    /* Wait for reboot (~10s) */
    for (volatile uint32_t i = 0; i < 1000000000; i++)
        ;

    return lte_power_on();
}

lte_state_t lte_get_state(void)
{
    return lte_state;
}

/* ==================== AT Command Interface ==================== */

int lte_send_at(const char *cmd, char *response, uint16_t resp_size, uint32_t timeout_ms)
{
    uint32_t start = 0;  /* Should use a SysTick-based millis() in production */
    at_rsp_len = 0;
    at_rsp_ready = 0;

    /* Send AT command with CR LF */
    uart8_puts(cmd);
    uart8_puts("\r\n");

    /* Wait for response */
    uint32_t count = timeout_ms * 10000;  /* Approximate loop timeout */
    while (!at_rsp_ready && count--) {
        for (volatile uint32_t i = 0; i < 10; i++)
            ;
    }

    if (!at_rsp_ready) {
        return -1;  /* Timeout */
    }

    /* Copy response */
    uint16_t len = at_rsp_len < (resp_size - 1) ? at_rsp_len : (resp_size - 1);
    memcpy(response, at_rsp_buf, len);
    response[len] = '\0';

    /* Check for OK or ERROR */
    if (strstr(response, "OK") != NULL) {
        return 0;
    }
    if (strstr(response, "ERROR") != NULL) {
        return -2;
    }

    return 0;
}

int lte_send_at_raw(const uint8_t *data, uint16_t len, uint8_t *response, uint16_t resp_size, uint32_t timeout_ms)
{
    /* Use DMA for large transfers */
    uart8_write_dma(data, len);

    uint32_t count = timeout_ms * 10000;
    while (!at_rsp_ready && count--) {
        for (volatile uint32_t i = 0; i < 10; i++)
            ;
    }

    if (!at_rsp_ready) return -1;

    uint16_t copy_len = at_rsp_len < (resp_size - 1) ? at_rsp_len : (resp_size - 1);
    memcpy(response, at_rsp_buf, copy_len);
    response[copy_len] = '\0';
    return 0;
}

/* ==================== Network Operations ==================== */

int lte_configure_apn(const char *apn, const char *user, const char *pass)
{
    char cmd[256];

    /* Define PDP context */
    snprintf(cmd, sizeof(cmd), "AT+CGDCONT=1,\"IP\",\"%s\"", apn);
    int ret = lte_send_at(cmd, at_rsp_buf, LTE_RSP_BUF_SIZE, 3000);
    if (ret != 0) return -1;

    /* Set authentication if provided */
    if (user && user[0] != '\0') {
        snprintf(cmd, sizeof(cmd), "AT+UAUTHREQ=1,1,\"%s\",\"%s\"", pass ? pass : "", user);
        ret = lte_send_at(cmd, at_rsp_buf, LTE_RSP_BUF_SIZE, 3000);
        if (ret != 0) return -2;
    }

    return 0;
}

int lte_connect(void)
{
    lte_state = LTE_STATE_CONNECTING;

    /* Activate PDP context */
    int ret = lte_send_at("AT+CGACT=1,1", at_rsp_buf, LTE_RSP_BUF_SIZE, 30000);
    if (ret != 0) {
        lte_state = LTE_STATE_ERROR;
        return -1;
    }

    /* Verify we got an IP address */
    ret = lte_send_at("AT+CGPADDR=1", at_rsp_buf, LTE_RSP_BUF_SIZE, 5000);
    if (ret != 0) {
        lte_state = LTE_STATE_ERROR;
        return -2;
    }

    lte_state = LTE_STATE_CONNECTED;
    return 0;
}

int lte_disconnect(void)
{
    int ret = lte_send_at("AT+CGACT=0,1", at_rsp_buf, LTE_RSP_BUF_SIZE, 10000);
    lte_state = LTE_STATE_INITIALIZING;
    return ret;
}

int lte_get_signal_quality(lte_signal_t *signal)
{
    int ret = lte_send_at("AT+CSQ", at_rsp_buf, LTE_RSP_BUF_SIZE, 3000);
    if (ret != 0) return -1;

    /* Parse +CSQ: <rssi>,<ber> */
    char *csq = strstr(at_rsp_buf, "+CSQ:");
    if (csq) {
        csq += 6;
        signal->rssi = (int8_t)atoi(csq);
        char *comma = strchr(csq, ',');
        if (comma) {
            signal->ber = (uint8_t)atoi(comma + 1);
        }
    }

    return 0;
}

int lte_check_registration(void)
{
    int ret = lte_send_at("AT+CREG?", at_rsp_buf, LTE_RSP_BUF_SIZE, 3000);
    if (ret != 0) return -1;

    /* Parse +CREG: <n>,<stat> */
    char *creg = strstr(at_rsp_buf, "+CREG:");
    if (creg) {
        creg += 7;
        char *comma = strchr(creg, ',');
        if (comma) {
            int stat = atoi(comma + 1);
            return stat;  /* Returns lte_net_state_t value */
        }
    }

    return -2;
}

int lte_get_network_info(lte_network_t *info)
{
    /* Get operator name */
    int ret = lte_send_at("AT+COPS?", at_rsp_buf, LTE_RSP_BUF_SIZE, 3000);
    if (ret == 0) {
        char *cops = strstr(at_rsp_buf, "+COPS:");
        if (cops) {
            /* Parse +COPS: <mode>,<format>,"<oper>" */
            char *quote1 = strchr(cops, '"');
            if (quote1) {
                char *quote2 = strchr(quote1 + 1, '"');
                if (quote2) {
                    uint16_t len = quote2 - quote1 - 1;
                    if (len >= sizeof(info->operator_name))
                        len = sizeof(info->operator_name) - 1;
                    memcpy(info->operator_name, quote1 + 1, len);
                    info->operator_name[len] = '\0';
                }
            }
        }
    }

    /* Get signal quality */
    lte_get_signal_quality(&info->signal);

    /* Get network registration status */
    info->net_state = lte_check_registration();

    /* Get access technology */
    ret = lte_send_at("AT+CGREG?", at_rsp_buf, LTE_RSP_BUF_SIZE, 3000);
    /* Parse access technology from network info */

    return 0;
}

/* ==================== MQTT Operations ==================== */

int lte_mqtt_connect(const lte_mqtt_config_t *config)
{
    char cmd[256];
    int ret;

    /* Configure MQTT broker */
    snprintf(cmd, sizeof(cmd), "AT+UMQTTC=0,\"%s\",%u", config->broker, config->port);
    ret = lte_send_at(cmd, at_rsp_buf, LTE_RSP_BUF_SIZE, 5000);
    if (ret != 0) return -1;

    /* Set client ID */
    snprintf(cmd, sizeof(cmd), "AT+UMQTTC=1,\"%s\"", config->client_id);
    ret = lte_send_at(cmd, at_rsp_buf, LTE_RSP_BUF_SIZE, 3000);
    if (ret != 0) return -2;

    /* Set credentials if provided */
    if (config->username[0] != '\0') {
        snprintf(cmd, sizeof(cmd), "AT+UMQTTC=2,\"%s\",\"%s\"", config->username, config->password);
        ret = lte_send_at(cmd, at_rsp_buf, LTE_RSP_BUF_SIZE, 3000);
        if (ret != 0) return -3;
    }

    /* Set keep-alive */
    snprintf(cmd, sizeof(cmd), "AT+UMQTTC=3,%u", config->keepalive);
    ret = lte_send_at(cmd, at_rsp_buf, LTE_RSP_BUF_SIZE, 3000);
    if (ret != 0) return -4;

    /* Connect to broker */
    ret = lte_send_at("AT+UMQTTC=10", at_rsp_buf, LTE_RSP_BUF_SIZE, 15000);
    if (ret != 0) return -5;

    return 0;
}

int lte_mqtt_disconnect(void)
{
    return lte_send_at("AT+UMQTTC=11", at_rsp_buf, LTE_RSP_BUF_SIZE, 5000);
}

int lte_mqtt_publish(const char *topic, const uint8_t *payload, uint16_t len, uint8_t qos)
{
    char cmd[256];

    /* AT+UMQTTP=<qos>,0,<topic_len>,"<topic>","" */
    snprintf(cmd, sizeof(cmd), "AT+UMQTTP=%u,0,%u,\"%s\"", qos, len, topic);

    /* Send command header first */
    int ret = lte_send_at(cmd, at_rsp_buf, LTE_RSP_BUF_SIZE, 5000);
    if (ret != 0) return -1;

    /* Send payload as raw data */
    ret = lte_send_at_raw(payload, len, (uint8_t *)at_rsp_buf, LTE_RSP_BUF_SIZE, 5000);
    if (ret != 0) return -2;

    return 0;
}

int lte_mqtt_subscribe(const char *topic, uint8_t qos)
{
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "AT+UMQTTS=%u,\"%s\"", qos, topic);
    return lte_send_at(cmd, at_rsp_buf, LTE_RSP_BUF_SIZE, 5000);
}

/* ==================== GNSS Pass-through ==================== */

int lte_gnss_enable(void)
{
    /* Enable LARA-R6 internal GNSS */
    int ret = lte_send_at("AT+UGPS=1,1,1", at_rsp_buf, LTE_RSP_BUF_SIZE, 5000);
    if (ret != 0) return -1;

    /* Enable GNSS data output on UART */
    ret = lte_send_at("AT+UGGGA=1", at_rsp_buf, LTE_RSP_BUF_SIZE, 3000);
    return ret;
}

int lte_gnss_disable(void)
{
    return lte_send_at("AT+UGPS=0", at_rsp_buf, LTE_RSP_BUF_SIZE, 3000);
}