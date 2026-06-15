/*
 * uart_fp.h - FPC1025 Fingerprint Sensor UART Driver Header
 * CyberGuard Hardware Security Token
 */

#ifndef UART_FP_H
#define UART_FP_H

#include <stdint.h>
#include <stddef.h>

/* FPC1025 Command Set */
#define FP_CMD_HW_ID            0x10
#define FP_CMD_INIT             0x01
#define FP_CMD_CAPTURE          0x03
#define FP_CMD_ENROLL           0x06
#define FP_CMD_VERIFY           0x07
#define FP_CMD_DELETE           0x08
#define FP_CMD_DELETE_ALL       0x09
#define FP_CMD_GET_TEMPLATE     0x0A
#define FP_CMD_SET_BAUD         0x0B
#define FP_CMD_SLEEP            0x0C
#define FP_CMD_WAKE             0x0D
#define FP_CMD_GET_IMAGE_QUALITY 0x0E
#define FP_CMD_GET_VERSION      0x0F

/* FPC1025 Response Codes */
#define FP_RSP_ACK              0xA0
#define FP_RSP_NACK             0xA1
#define FP_RSP_DATA             0xA2
#define FP_RSP_ERROR            0xA3

/* FPC1025 Error Codes */
#define FP_ERR_NONE             0x00
#define FP_ERR_TIMEOUT          0x01
#define FP_ERR_INVALID_CMD      0x02
#define FP_ERR_INVALID_PARAM    0x03
#define FP_ERR_NO_FINGER        0x04
#define FP_ERR_LOW_QUALITY      0x05
#define FP_ERR_NOT_ENOUGH      0x06
#define FP_ERR_TEMPLATE_FULL    0x07
#define FP_ERR_NO_MATCH         0x08
#define FP_ERR_COMM             0x09

/* FPC1025 Template/Slot limits */
#define FP_MAX_TEMPLATES        10
#define FP_MAX_ENROLL_SAMPLES   6
#define FP_IMAGE_WIDTH          160
#define FP_IMAGE_HEIGHT         160
#define FP_IMAGE_SIZE           (FP_IMAGE_WIDTH * FP_IMAGE_HEIGHT)

/* FPC1025 UART frame format:
 * [STX(0xAA)] [LEN_H] [LEN_L] [CMD] [DATA...] [CHK]
 * Response:   [STX(0xAA)] [LEN_H] [LEN_L] [RSP] [DATA...] [CHK]
 */
#define FP_STX                  0xAA

/**
 * Initialize FPC1025 fingerprint sensor over UART1
 * @return 0 on success, negative on error
 */
int fp_init(void);

/**
 * Capture a fingerprint image
 * @param image_buf Buffer for 160x160 grayscale image (25600 bytes)
 * @param image_len Size of buffer, updated with actual image size
 * @return 0 on success, negative on error
 */
int fp_capture(uint8_t *image_buf, size_t *image_len);

/**
 * Enroll a fingerprint into a template slot
 * @param slot Template slot (0-9)
 * @param samples Number of enrollment samples (1-6)
 * @return 0 on success, negative on error
 */
int fp_enroll(uint8_t slot, uint8_t samples);

/**
 * Verify a fingerprint against a template
 * @param slot Template slot to verify against
 * @param score Pointer to store match score (0-65535)
 * @return 0 on match, negative on no match or error
 */
int fp_verify(uint8_t slot, uint16_t *score);

/**
 * Delete a fingerprint template
 * @param slot Template slot (0-9), or 0xFF for all
 * @return 0 on success, negative on error
 */
int fp_delete(uint8_t slot);

/**
 * Get the number of stored templates
 * @param count Pointer to store template count
 * @return 0 on success, negative on error
 */
int fp_get_template_count(uint8_t *count);

/**
 * Put FPC1025 into low-power sleep mode
 * @return 0 on success, negative on error
 */
int fp_sleep(void);

/**
 * Wake FPC1025 from sleep mode
 * @return 0 on success, negative on error
 */
int fp_wakeup(void);

/**
 * Get FPC1025 hardware ID and version
 * @param version_buf Buffer for version string (min 32 bytes)
 * @return 0 on success, negative on error
 */
int fp_get_version(uint8_t *version_buf);

#endif /* UART_FP_H */