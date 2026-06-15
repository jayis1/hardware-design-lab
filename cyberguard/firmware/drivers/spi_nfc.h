/*
 * spi_nfc.h - PN7150 NFC Controller Driver Header
 * CyberGuard Hardware Security Token
 */

#ifndef SPI_NFC_H
#define SPI_NFC_H

#include <stdint.h>
#include <stddef.h>

/* PN7150 SPI Protocol Constants */
#define PN7150_SPI_PREAMBLE      0x00
#define PN7150_SPI_STARTCODE     0xFF
#define PN7150_SPI_ACK           0x01
#define PN7150_SPI_NACK          0x00
#define PN7150_SPI_ERR           0x02

/* PN7150 Core Commands */
#define PN7150_CMD_SYSTEM_RESET          0x01
#define PN7150_CMD_GET_FW_VERSION        0x02
#define PN7150_CMD_GET_PROD_CODE         0x03
#define PN7150_CMD_GET_PROD_ID           0x04
#define PN7150_CMD_GET_SECURE_FW_VERSION 0x05
#define PN7150_CMD_WRITE_REGISTER        0x08
#define PN7150_CMD_READ_REGISTER         0x0A

/* PN7150 RF Commands (through NCI) */
#define PN7150_NCI_CORE_RESET            0x2000
#define PN7150_NCI_CORE_INIT             0x2001
#define PN7150_NCI_CORE_SET_CONFIG       0x2002
#define PN7150_NCI_CORE_GET_CONFIG       0x2003
#define PN7150_NCI_CORE_CONN_CREATE      0x2004
#define PN7150_NCI_CORE_CONN_CLOSE       0x2005
#define PN7150_NCI_CORE_CONN_CREDITS     0x2006
#define PN7150_NCI_DISCOVER_MAP          0x2100
#define PN7150_NCI_RF_DISCOVER           0x2103
#define PN7150_NCI_RF_DEACTIVATE         0x2106
#define PN7150_NCI_RF_FIELD              0x2107

/* NFC Forum Tag Types */
#define NFC_TYPE_A           0x00
#define NFC_TYPE_B           0x01
#define NFC_TYPE_F           0x02

/* CTAP2-over-NFC APDU Constants */
#define NFC_APDU_CLA         0x00
#define NFC_APDU_INS         0xA4  /* SELECT */
#define NFC_APDU_P1          0x04
#define NFC_APDU_P2          0x00

/* FIDO2 AID */
#define FIDO2_AID_SIZE       8
static const uint8_t FIDO2_AID[FIDO2_AID_SIZE] = {0xA0, 0x00, 0x00, 0x06, 0x47, 0x2F, 0x00, 0x01};

/* Maximum NFC frame sizes */
#define PN7150_MAX_FRAME_LEN   264
#define PN7150_NCI_MAX_PAYLOAD  255

/**
 * Initialize PN7150 NFC controller
 * @return 0 on success, negative on error
 */
int nfc_init(void);

/**
 * Send NCI command to PN7150
 * @param group NCI group ID
 * @param opcode NCI opcode
 * @param payload Payload data
 * @param payload_len Payload length
 * @param rsp_buf Response buffer
 * @param rsp_len Pointer to response buffer size, updated with actual length
 * @return 0 on success, negative on error
 */
int nfc_nci_cmd(uint8_t group, uint8_t opcode,
                const uint8_t *payload, size_t payload_len,
                uint8_t *rsp_buf, size_t *rsp_len);

/**
 * Start NFC field (polling for tags)
 * @return 0 on success, negative on error
 */
int nfc_start_discovery(void);

/**
 * Stop NFC field
 * @return 0 on success, negative on error
 */
int nfc_stop_discovery(void);

/**
 * Send APDU command over NFC
 * @param cla APDU CLA byte
 * @param ins APDU INS byte
 * @param p1 APDU P1 byte
 * @param p2 APDU P2 byte
 * @param data APDU data
 * @param data_len Data length
 * @param rsp Response buffer
 * @param rsp_len Response length (updated)
 * @return 0 on success, negative on error
 */
int nfc_send_apdu(uint8_t cla, uint8_t ins, uint8_t p1, uint8_t p2,
                  const uint8_t *data, size_t data_len,
                  uint8_t *rsp, size_t *rsp_len);

/**
 * Check if NFC field is active (tag detected)
 * @return 1 if field active, 0 if not, negative on error
 */
int nfc_field_active(void);

/**
 * Process NFC IRQ (call from EXTI handler)
 * @return 0 on success, negative on error
 */
int nfc_irq_handler(void);

/**
 * De-initialize PN7150 (power down)
 * @return 0 on success, negative on error
 */
int nfc_deinit(void);

#endif /* SPI_NFC_H */