/*
 * i2c_se.h - A71CH Secure Element I2C Driver Header
 * CyberGuard Hardware Security Token
 */

#ifndef I2C_SE_H
#define I2C_SE_H

#include <stdint.h>
#include <stddef.h>

/* A71CH I2C address (7-bit) */
#define A71CH_ADDR           0x48

/* A71CH Command Set */
#define A71CH_CMD_INIT        0x10
#define A71CH_CMD_GET_INFO    0x11
#define A71CH_CMD_GEN_KEYPAIR 0x20
#define A71CH_CMD_SET_KEY     0x21
#define A71CH_CMD_SIGN        0x30
#define A71CH_CMD_VERIFY      0x31
#define A71CH_CMD_STORE_CERT  0x40
#define A71CH_CMD_READ_CERT   0x41
#define A71CH_CMD_DELETE      0x50
#define A71CH_CMD_RANDOM      0x60
#define A71CH_CMD_ECHO        0x70

/* A71CH Response Codes */
#define A71CH_RSP_OK         0x00
#define A71CH_RSP_ERR_CRYPTO 0x01
#define A71CH_RSP_ERR_ACCESS 0x02
#define A71CH_RSP_ERR_STATE  0x03
#define A71CH_RSP_ERR_PARAM  0x04
#define A71CH_RSP_ERR_MEMORY 0x05

/* A71CH Key Slot Definitions */
#define A71CH_SLOT_COUNT     10
#define A71CH_SLOT_FIRST     0x00
#define A71CH_SLOT_LAST      0x09

/* A71CH Curve Types */
#define A71CH_CURVE_P256     0x00
#define A71CH_CURVE_P384     0x01
#define A71CH_CURVE_ED25519  0x02

/* A71CH Max buffer sizes */
#define A71CH_MAX_TX_LEN     512
#define A71CH_MAX_RX_LEN     512
#define A71CH_MAX_SIG_LEN    72    /* P-256 signature */
#define A71CH_MAX_CERT_LEN   1024

/* CTAP2 attestation key slot */
#define A71CH_ATTEST_SLOT    0x00
/* Resident key slots */
#define A71CH_RK_SLOT_START  0x01
#define A71CH_RK_SLOT_END    0x09

/**
 * Initialize A71CH secure element over I2C
 * @return 0 on success, negative on error
 */
int se_init(void);

/**
 * Generate a key pair in specified slot
 * @param slot Key slot (0-9)
 * @param curve Curve type (P256, P384, ED25519)
 * @param pubkey_out Buffer for public key output (64 bytes for P256)
 * @param pubkey_len Size of pubkey_out buffer
 * @return 0 on success, negative on error
 */
int se_gen_keypair(uint8_t slot, uint8_t curve, uint8_t *pubkey_out, size_t pubkey_len);

/**
 * Sign data with private key in specified slot
 * @param slot Key slot (0-9)
 * @param data Data to sign
 * @param data_len Length of data
 * @param sig_out Signature output buffer
 * @param sig_len Size of signature buffer, updated with actual length
 * @return 0 on success, negative on error
 */
int se_sign(uint8_t slot, const uint8_t *data, size_t data_len,
            uint8_t *sig_out, size_t *sig_len);

/**
 * Verify a signature
 * @param slot Key slot containing public key
 * @param data Original data
 * @param data_len Length of data
 * @param sig Signature to verify
 * @param sig_len Length of signature
 * @return 0 on valid signature, negative on error/invalid
 */
int se_verify(uint8_t slot, const uint8_t *data, size_t data_len,
              const uint8_t *sig, size_t sig_len);

/**
 * Get random bytes from A71CH TRNG
 * @param buf Output buffer
 * @param len Number of random bytes
 * @return 0 on success, negative on error
 */
int se_random(uint8_t *buf, size_t len);

/**
 * Store a certificate in A71CH
 * @param slot Certificate slot
 * @param cert Certificate data (DER)
 * @param cert_len Certificate length
 * @return 0 on success, negative on error
 */
int se_store_cert(uint8_t slot, const uint8_t *cert, size_t cert_len);

/**
 * Read a certificate from A71CH
 * @param slot Certificate slot
 * @param cert_out Buffer for certificate data
 * @param cert_len Buffer size, updated with actual length
 * @return 0 on success, negative on error
 */
int se_read_cert(uint8_t slot, uint8_t *cert_out, size_t *cert_len);

/**
 * Get A71CH device info
 * @param info_out Buffer for info string
 * @param info_len Buffer size
 * @return 0 on success, negative on error
 */
int se_get_info(uint8_t *info_out, size_t info_len);

#endif /* I2C_SE_H */