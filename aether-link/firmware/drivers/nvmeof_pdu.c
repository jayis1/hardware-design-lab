// nvmeof_pdu.c — NVMe-oF PDU Engine Implementation
// Full hardware-accelerated NVMe-oF/TCP PDU processing for Aether-Link.
// Handles command capsule construction, response parsing, CRC32C validation,
// and zero-copy DMA coordination with the PCIe block.

#include "nvmeof_pdu.h"
#include "toe_driver.h"
#include "../registers.h"
#include "../board.h"
#include <string.h>

// --- Hardware Register Access ---
static volatile uint32_t * const nvmeof_regs = (volatile uint32_t *)NVMEOF_REG_BASE;
static uint8_t *data_buffer;
static uint32_t data_buf_base;

static inline uint32_t nvmeof_reg_read(uint32_t offset) {
    return nvmeof_regs[offset / 4];
}

static inline void nvmeof_reg_write(uint32_t offset, uint32_t value) {
    nvmeof_regs[offset / 4] = value;
}

// CRC32C function (declared in toe_driver.h)
extern uint32_t crc32c(const uint8_t *data, size_t len);

// --- Initialization ---
int nvmeof_pdu_init(uint32_t ddr4_base) {
    data_buf_base = ddr4_base;
    data_buffer = (uint8_t *)ddr4_base;

    // Reset PDU engine
    nvmeof_reg_write(NVMEOF_REG_CTRL, NVMEOF_CTRL_RESET);
    for (volatile int i = 0; i < 1000; i++);
    nvmeof_reg_write(NVMEOF_REG_CTRL, 0x00);

    // Configure data buffer base in DDR4
    nvmeof_reg_write(NVMEOF_REG_DATA_BUF_BASE, ddr4_base);

    // Set max connections
    nvmeof_reg_write(NVMEOF_REG_MAX_CONNS, 256);

    // Configure digest mode: enable both header and data digests
    nvmeof_reg_write(NVMEOF_REG_DIGEST_MODE,
                     NVMEOF_CTRL_HDGST_EN | NVMEOF_CTRL_DDGST_EN);

    // Set PDU timeout: 30 seconds (30,000,000 µs)
    nvmeof_reg_write(NVMEOF_REG_TIMEOUT_US, 30000000);

    // Set retry count: 3
    nvmeof_reg_write(NVMEOF_REG_RETRY_COUNT, 3);

    // Enable PDU engine with zero-copy and in-order completion
    nvmeof_reg_write(NVMEOF_REG_CTRL,
                     NVMEOF_CTRL_ENABLE | NVMEOF_CTRL_HDGST_EN |
                     NVMEOF_CTRL_DDGST_EN | NVMEOF_CTRL_ZERO_COPY |
                     NVMEOF_CTRL_IN_ORDER);

    return 0;
}

// --- Send NVMe Command as NVMe-oF Command Capsule ---
int nvmeof_pdu_send_cmd(uint8_t conn_id, const uint8_t *sqe,
                        uint32_t data_len, const uint8_t *data) {

    if (conn_id >= 256 || sqe == NULL) return -1;

    // Build Command Capsule in DDR4 PDU assembly buffer
    nvmeof_cmd_capsule_t *capsule = (nvmeof_cmd_capsule_t *)data_buffer;

    // Zero the capsule
    memset(capsule, 0, sizeof(nvmeof_cmd_capsule_t));

    // Initialize PDU header
    capsule->hdr.pdu_type = NVMEOF_PDU_CAP_CMD;
    capsule->hdr.version = 0x01;  // NVMe-oF 1.1
    capsule->hdr.pdu_data_offset = sizeof(nvmeof_pdu_header_t) + 64;
    capsule->hdr.pdu_data_length = (uint16_t)data_len;

    // Set PDU-specific fields for command capsule
    // pdu_specific0: Queue ID (high 16 bits) + Command ID (low 16 bits)
    capsule->hdr.pdu_specific0 = (conn_id << 16) | (sqe[2] | (sqe[3] << 8));
    // pdu_specific1: Namespace ID
    capsule->hdr.pdu_specific1 = sqe[4] | (sqe[5] << 8) | (sqe[6] << 16) | (sqe[7] << 24);

    // Copy SQE (64 bytes) into capsule
    memcpy(&capsule->sqe_opcode, sqe, 64);

    // Compute header digest (CRC32C of header, excluding digest field itself)
    capsule->hdr.header_digest = crc32c((uint8_t *)capsule,
                                         sizeof(nvmeof_pdu_header_t) - 4);

    // Copy data after capsule if present
    uint32_t total_len = sizeof(nvmeof_cmd_capsule_t);
    if (data != NULL && data_len > 0) {
        memcpy(data_buffer + sizeof(nvmeof_cmd_capsule_t), data, data_len);
        capsule->data_digest = crc32c(data, data_len);
        total_len += data_len;
    } else {
        capsule->data_digest = 0;
    }

    // Send via TOE
    int ret = toe_send_data(conn_id, data_buffer, total_len);
    return ret;
}

// --- Receive NVMe-oF Response Capsule ---
int nvmeof_pdu_recv_resp(uint8_t conn_id, uint8_t *cqe, uint32_t *data_len,
                         uint8_t *data_buf, uint32_t max_data) {

    if (conn_id >= 256 || cqe == NULL) return -1;

    // Receive data from TOE into PDU assembly buffer
    uint32_t actual_len = 0;
    int ret = toe_recv_data(conn_id, data_buffer, 65536, &actual_len);
    if (ret != 0 || actual_len < sizeof(nvmeof_resp_capsule_t)) return -2;

    // Parse Response Capsule
    nvmeof_resp_capsule_t *capsule = (nvmeof_resp_capsule_t *)data_buffer;

    // Validate PDU type
    if (capsule->hdr.pdu_type != NVMEOF_PDU_CAP_RESP &&
        capsule->hdr.pdu_type != NVMEOF_PDU_D2H_DATA) {
        return -3;
    }

    // Validate header digest
    uint32_t computed_digest = crc32c((uint8_t *)capsule,
                                       sizeof(nvmeof_pdu_header_t) - 4);
    if (computed_digest != capsule->hdr.header_digest) {
        nvmeof_reg_write(NVMEOF_REG_ERROR_CNT,
                         nvmeof_reg_read(NVMEOF_REG_ERROR_CNT) + 1);
        return -4;  // Header corruption
    }

    // Copy CQE (16 bytes) to caller's buffer
    memcpy(cqe, &capsule->cqe_dw0, 16);

    // Extract data if present (D2H data PDU)
    if (capsule->hdr.pdu_data_length > 0 && data_buf != NULL) {
        uint32_t copy_len = capsule->hdr.pdu_data_length;
        if (copy_len > max_data) copy_len = max_data;

        uint8_t *pdu_data = data_buffer + capsule->hdr.pdu_data_offset;
        memcpy(data_buf, pdu_data, copy_len);

        // Validate data digest if present
        if (capsule->data_digest != 0) {
            uint32_t computed_data_digest = crc32c(pdu_data,
                                                    capsule->hdr.pdu_data_length);
            if (computed_data_digest != capsule->data_digest) {
                nvmeof_reg_write(NVMEOF_REG_ERROR_CNT,
                                 nvmeof_reg_read(NVMEOF_REG_ERROR_CNT) + 1);
                return -5;  // Data corruption
            }
        }

        if (data_len) *data_len = copy_len;
    } else {
        if (data_len) *data_len = 0;
    }

    return 0;
}

// --- Process Incoming PDU (called from TOE interrupt context) ---
void nvmeof_pdu_process_incoming(uint8_t conn_id) {
    // The hardware PDU engine automatically processes incoming PDUs:
    // 1. Validates CRC32C header digest
    // 2. Validates CRC32C data digest (if present)
    // 3. Extracts NVMe CQE from response capsule
    // 4. Posts CQE to host Completion Queue via PCIe
    // 5. If D2H data PDU, DMAs data to host memory via PRP list
    // 6. Generates MSI-X interrupt to host

    // This function handles software-side processing:
    uint32_t status = nvmeof_reg_read(NVMEOF_REG_STATUS);

    if (status & NVMEOF_STATUS_PDU_PROC) {
        // PDU processed successfully by hardware
        // IOPS counter is incremented automatically by hardware
    }

    if (status & NVMEOF_STATUS_ERROR) {
        // Log error for diagnostics
        uint32_t errors = nvmeof_reg_read(NVMEOF_REG_ERROR_CNT);
        // Error details are in the error log ring buffer in DDR4
    }

    if (status & NVMEOF_STATUS_DIGEST_ERR) {
        // Digest error — PDU discarded, target will retransmit
    }

    if (status & NVMEOF_STATUS_CAPSULE_ERR) {
        // Capsule format error — protocol violation
    }
}

// --- Statistics ---
uint64_t nvmeof_pdu_get_iops(void) {
    uint32_t lo = nvmeof_reg_read(NVMEOF_REG_IOPS_CNT_LO);
    uint32_t hi = nvmeof_reg_read(NVMEOF_REG_IOPS_CNT_HI);
    return ((uint64_t)hi << 32) | lo;
}

uint32_t nvmeof_pdu_get_errors(void) {
    return nvmeof_reg_read(NVMEOF_REG_ERROR_CNT);
}

// --- NVMe-oF Fabric Connect (ICReq/ICResp exchange) ---
// This is called when a new TCP connection is established to perform
// the NVMe-oF Fabric Connect sequence.

int nvmeof_fabric_connect(uint8_t conn_id, uint16_t cntlid, uint16_t subnqn_len,
                          const char *subnqn, uint16_t hostnqn_len,
                          const char *hostnqn) {

    // Build ICReq PDU
    nvmeof_pdu_header_t *icreq = (nvmeof_pdu_header_t *)data_buffer;
    memset(icreq, 0, sizeof(nvmeof_pdu_header_t));

    icreq->pdu_type = NVMEOF_PDU_ICREQ;
    icreq->version = 0x01;
    icreq->pdu_specific0 = (cntlid << 16) | 0x0001;  // CNTLID + PFV=1
    icreq->pdu_specific1 = subnqn_len;
    icreq->pdu_specific2 = hostnqn_len;

    // Copy SubNQN and HostNQN after header
    uint32_t offset = sizeof(nvmeof_pdu_header_t);
    if (subnqn != NULL && subnqn_len > 0) {
        memcpy(data_buffer + offset, subnqn, subnqn_len);
        offset += subnqn_len;
    }
    if (hostnqn != NULL && hostnqn_len > 0) {
        memcpy(data_buffer + offset, hostnqn, hostnqn_len);
        offset += hostnqn_len;
    }

    icreq->pdu_data_offset = sizeof(nvmeof_pdu_header_t);
    icreq->pdu_data_length = offset - sizeof(nvmeof_pdu_header_t);
    icreq->header_digest = crc32c((uint8_t *)icreq, sizeof(nvmeof_pdu_header_t) - 4);

    // Send ICReq via TOE
    int ret = toe_send_data(conn_id, data_buffer, offset);
    if (ret < 0) return ret;

    // Wait for ICResp (in production, this would be interrupt-driven)
    // For bare-metal, we poll with timeout
    uint32_t timeout = 5000;  // 5 second timeout
    uint8_t cqe[16];
    uint32_t resp_len = 0;

    while (timeout > 0) {
        ret = nvmeof_pdu_recv_resp(conn_id, cqe, &resp_len, NULL, 0);
        if (ret == 0) {
            // Check if this is an ICResp
            nvmeof_pdu_header_t *icresp = (nvmeof_pdu_header_t *)data_buffer;
            if (icresp->pdu_type == NVMEOF_PDU_ICRESP) {
                // Fabric Connect successful
                // pdu_specific0 contains status (0 = success)
                return (icresp->pdu_specific0 & 0xFFFF) == 0 ? 0 : -10;
            }
        }
        // Small delay between polls
        for (volatile int i = 0; i < 200000; i++);  // ~1ms at 200MHz
        timeout--;
    }

    return -11;  // Timeout waiting for ICResp
}
