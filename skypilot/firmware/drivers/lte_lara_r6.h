/*
 * SkyPilot — u-blox LARA-R6 LTE Modem Driver Header
 * UART8 with DMA ring buffer on STM32H743VIT6
 */

#ifndef SKYPILOT_LTE_LARA_R6_H
#define SKYPILOT_LTE_LARA_R6_H

#include <stdint.h>
#include <stdbool.h>

/* AT command response buffer sizes */
#define LTE_RSP_BUF_SIZE    1024
#define LTE_RING_BUF_SIZE   4096

/* LARA-R6 Power Control */
#define LTE_PWR_ON_LOW()    (GPIOA->BSRR = (1UL << (LTE_PWR_ON_PIN + 16)))
#define LTE_PWR_ON_HIGH()   (GPIOA->BSRR = (1UL << LTE_PWR_ON_PIN))
#define LTE_STATUS_READ()    ((GPIOD->IDR >> LTE_STATUS_PIN) & 1)

/* Network registration states */
typedef enum {
    LTE_NET_NOT_REGISTERED = 0,
    LTE_NET_REGISTERED_HOME = 1,
    LTE_NET_SEARCHING = 2,
    LTE_NET_DENIED = 3,
    LTE_NET_UNKNOWN = 4,
    LTE_NET_REGISTERED_ROAMING = 5,
} lte_net_state_t;

/* Connection states */
typedef enum {
    LTE_STATE_OFF,
    LTE_STATE_POWERING_ON,
    LTE_STATE_INITIALIZING,
    LTE_STATE_CONFIGURING,
    LTE_STATE_CONNECTING,
    LTE_STATE_CONNECTED,
    LTE_STATE_ERROR,
} lte_state_t;

/* Signal quality */
typedef struct {
    int8_t rssi;        /* dBm (approx: -113 + 2*rssi) */
    uint8_t ber;        /* Bit Error Rate (%) */
} lte_signal_t;

/* Network info */
typedef struct {
    lte_net_state_t net_state;
    char operator_name[32];
    char operator_num[8];
    uint8_t act;        /* 0=2G, 2=UTRAN, 4=E-UTRAN, 5=E-UTRAN NSA */
    lte_signal_t signal;
    char ip_addr[48];
    char apn[64];
    uint8_t cid;        /* Context ID */
} lte_network_t;

/* MQTT configuration */
typedef struct {
    char broker[128];
    uint16_t port;
    char client_id[64];
    char username[64];
    char password[64];
    uint16_t keepalive;
} lte_mqtt_config_t;

/* Function prototypes */

/* Initialization and power control */
int  lte_init(void);
int  lte_power_on(void);
int  lte_power_off(void);
int  lte_reset(void);
lte_state_t lte_get_state(void);

/* AT command interface */
int  lte_send_at(const char *cmd, char *response, uint16_t resp_size, uint32_t timeout_ms);
int  lte_send_at_raw(const uint8_t *data, uint16_t len, uint8_t *response, uint16_t resp_size, uint32_t timeout_ms);

/* Network operations */
int  lte_configure_apn(const char *apn, const char *user, const char *pass);
int  lte_connect(void);
int  lte_disconnect(void);
int  lte_get_network_info(lte_network_t *info);
int  lte_get_signal_quality(lte_signal_t *signal);
int  lte_check_registration(void);

/* MQTT operations */
int  lte_mqtt_connect(const lte_mqtt_config_t *config);
int  lte_mqtt_disconnect(void);
int  lte_mqtt_publish(const char *topic, const uint8_t *payload, uint16_t len, uint8_t qos);
int  lte_mqtt_subscribe(const char *topic, uint8_t qos);

/* GNSS pass-through */
int  lte_gnss_enable(void);
int  lte_gnss_disable(void);

/* UART DMA ring buffer */
void lte_uart_rx_irq_handler(void);
void lte_uart_tx_dma_irq_handler(void);

#endif /* SKYPILOT_LTE_LARA_R6_H */