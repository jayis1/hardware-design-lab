// nvmeof_pdu.h — NVMe-oF PDU Engine Driver for Aether-Link
// Handles NVMe-oF/TCP capsule construction, parsing, and CRC32C validation.

#ifndef NVMEOF_PDU_H
#define NVMEOF_PDU_H

#include <stdint.h>

// --- NVMe-oF/TCP PDU Types ---
#define NVMEOF_PDU_ICREQ     0x00
#define NVMEOF_PDU_ICRESP    0x01
#define NVMEOF_PDU_H2D_DATA  0x02
#define NVMEOF_PDU_D2H_DATA  0x03
#define NVMEOF_PDU_CAP_CMD   0x04
#define NVMEOF_PDU_CAP_RESP  0x05
#define NVMEOF_PDU_TERM_REQ  0x06
#define NVMEOF_PDU_TERM_RESP 0x07

// --- PDU Header Structure ---
typedef struct __attribute__((packed)) {
    uint8_t  pdu_type;
    uint8_t  flags;
    uint8_t  version;
    uint8_t  reserved0;
    uint16_t pdu_data_offset;
    uint16_t pdu_data_length;
    uint32_t pdu_specific0;
    uint32_t pdu_specific1;
    uint32_t pdu_specific2;
    uint32_t pdu_specific3;
    uint32_t pdu_specific4;
    uint32_t pdu_specific5;
    uint32_t pdu_specific6;
    uint32_t pdu_specific7;
    uint32_t pdu_specific8;
    uint32_t pdu_specific9;
    uint32_t header_digest;
} nvmeof_pdu_header_t;

// --- Command Capsule ---
typedef struct __attribute__((packed)) {
    nvmeof_pdu_header_t hdr;
    uint8_t  sqe_opcode;
    uint8_t  sqe_flags;
    uint16_t sqe_command_id;
    uint32_t sqe_nsid;
    uint64_t sqe_reserved;
    uint64_t sqe_metadata_ptr;
    uint64_t sqe_data_ptr[2];
    uint32_t sqe_data_len[2];
    uint32_t sqe_cdw10;
    uint32_t sqe_cdw11;
    uint32_t sqe_cdw12;
    uint32_t sqe_cdw13;
    uint32_t sqe_cdw14;
    uint32_t sqe_cdw15;
    uint32_t data_digest;
} nvmeof_cmd_capsule_t;

// --- Response Capsule ---
typedef struct __attribute__((packed)) {
    nvmeof_pdu_header_t hdr;
    uint32_t cqe_dw0;
    uint32_t cqe_dw1;
    uint16_t cqe_sq_head;
    uint16_t cqe_sq_id;
    uint16_t cqe_command_id;
    uint16_t cqe_status;
    uint32_t data_digest;
} nvmeof_resp_capsule_t;

// --- Function Prototypes ---
int  nvmeof_pdu_init(uint32_t data_buf_ddr4_base);
int  nvmeof_pdu_send_cmd(uint8_t conn_id, const uint8_t *sqe, uint32_t data_len,
                         const uint8_t *data);
int  nvmeof_pdu_recv_resp(uint8_t conn_id, uint8_t *cqe, uint32_t *data_len,
                          uint8_t *data_buf, uint32_t max_data);
void nvmeof_pdu_process_incoming(uint8_t conn_id);
uint64_t nvmeof_pdu_get_iops(void);
uint32_t nvmeof_pdu_get_errors(void);

#endif // NVMEOF_PDU_H
