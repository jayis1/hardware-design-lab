// toe_driver.c — TCP Offload Engine Driver Implementation
// Full hardware-accelerated TCP/IP stack management for Aether-Link.
// Target: MicroBlaze soft processor in XC7K325T FPGA fabric.
// Manages 256 concurrent TCP connections with TSO, LRO, and DCTCP.

#include "toe_driver.h"
#include "../registers.h"
#include "../board.h"
#include <string.h>

// --- Hardware Register Access ---
static volatile uint32_t * const toe_regs = (volatile uint32_t *)TOE_REG_BASE;

static inline uint32_t toe_reg_read(uint32_t offset) {
    return toe_regs[offset / 4];
}

static inline void toe_reg_write(uint32_t offset, uint32_t value) {
    toe_regs[offset / 4] = value;
}

// --- Global State ---
static toe_conn_state_t *conn_table;
static uint32_t conn_table_ddr4_base;
static uint8_t toe_initialized = 0;
static uint8_t port0_mac[6];
static uint8_t port1_mac[6];

// --- ARP Table ---
#define ARP_TABLE_SIZE      256
#define ARP_ENTRY_SIZE      16
#define ARP_ENTRY_VALID     0x01
#define ARP_ENTRY_STATIC    0x02

typedef struct {
    uint32_t ip_addr;
    uint8_t  mac_addr[6];
    uint16_t ttl_seconds;
    uint8_t  flags;
    uint8_t  padding;
} arp_entry_t;

static arp_entry_t *arp_table;

// --- CRC32C Table (Castagnoli polynomial 0x1EDC6F41) ---
static const uint32_t crc32c_table[256] = {
    0x00000000, 0xF26B8303, 0xE13B70F7, 0x1350F3F4,
    0xC79A971F, 0x35F1141C, 0x26A1E7E8, 0xD4CA64EB,
    0x8AD958CF, 0x78B2DBCC, 0x6BE22838, 0x9989AB3B,
    0x4D43CFD0, 0xBF284CD3, 0xAC78BF27, 0x5E133C24,
    0x105EC76F, 0xE235446C, 0xF165B798, 0x030E349B,
    0xD7C45070, 0x25AFD373, 0x36FF2087, 0xC494A384,
    0x9A879FA0, 0x68EC1CA3, 0x7BBCEF57, 0x89D76C54,
    0x5D1D08BF, 0xAF768BBC, 0xBC267848, 0x4E4DFB4B,
    0x20BD8EDE, 0xD2D60DDD, 0xC186FE29, 0x33ED7D2A,
    0xE72719C1, 0x154C9AC2, 0x061C6936, 0xF477EA35,
    0xAA64D611, 0x580F5512, 0x4B5FA6E6, 0xB93425E5,
    0x6DFE410E, 0x9F95C20D, 0x8CC531F9, 0x7EAEB2FA,
    0x30E349B1, 0xC288CAB2, 0xD1D83946, 0x23B3BA45,
    0xF779DEAE, 0x05125DAD, 0x1642AE59, 0xE4292D5A,
    0xBA3A117E, 0x4851927D, 0x5B016189, 0xA96AE28A,
    0x7DA08661, 0x8FCB0562, 0x9C9BF696, 0x6EF07595,
    0x417B1DBC, 0xB3109EBF, 0xA0406D4B, 0x522BEE48,
    0x86E18AA3, 0x748A09A0, 0x67DAFA54, 0x95B17957,
    0xCBA24573, 0x39C9C670, 0x2A993584, 0xD8F2B687,
    0x0C38D26C, 0xFE53516F, 0xED03A29B, 0x1F682198,
    0x5125DAD3, 0xA34E59D0, 0xB01EAA24, 0x42752927,
    0x96BF4DCC, 0x64D4CECF, 0x77843D3B, 0x85EFBE38,
    0xDBFC821C, 0x2997011F, 0x3AC7F2EB, 0xC8AC71E8,
    0x1C661503, 0xEE0D9600, 0xFD5D65F4, 0x0F36E6F7,
    0x61C69362, 0x93AD1061, 0x80FDE395, 0x72966096,
    0xA65C047D, 0x5437877E, 0x4767748A, 0xB50CF789,
    0xEB1FCBAD, 0x197448AE, 0x0A24BB5A, 0xF84F3859,
    0x2C855CB2, 0xDEEEDFB1, 0xCDBE2C45, 0x3FD5AF46,
    0x7198540D, 0x83F3D70E, 0x90A324FA, 0x62C8A7F9,
    0xB602C312, 0x44694011, 0x5739B3E5, 0xA55230E6,
    0xFB410CC2, 0x092A8FC1, 0x1A7A7C35, 0xE811FF36,
    0x3CDB9BDD, 0xCEB018DE, 0xDDE0EB2A, 0x2F8B6829,
    0x82F63B78, 0x709DB87B, 0x63CD4B8F, 0x91A6C88C,
    0x456CAC67, 0xB7072F64, 0xA457DC90, 0x563C5F93,
    0x082F63B7, 0xFA44E0B4, 0xE9141340, 0x1B7F9043,
    0xCFB5F4A8, 0x3DDE77AB, 0x2E8E845F, 0xDCE5075C,
    0x92A8FC17, 0x60C37F14, 0x73938CE0, 0x81F80FE3,
    0x55326B08, 0xA759E80B, 0xB4091BFF, 0x466298FC,
    0x1871A4D8, 0xEA1A27DB, 0xF94AD42F, 0x0B21572C,
    0xDFEB33C7, 0x2D80B0C4, 0x3ED04330, 0xCCBBC033,
    0xA24BB5A6, 0x502036A5, 0x4370C551, 0xB11B4652,
    0x65D122B9, 0x97BAA1BA, 0x84EA524E, 0x7681D14D,
    0x2892ED69, 0xDAF96E6A, 0xC9A99D9E, 0x3BC21E9D,
    0xEF087A76, 0x1D63F975, 0x0E330A81, 0xFC588982,
    0xB21572C9, 0x407EF1CA, 0x532E023E, 0xA145813D,
    0x758FE5D6, 0x87E466D5, 0x94B49521, 0x66DF1622,
    0x38CC2A06, 0xCAA7A905, 0xD9F75AF1, 0x2B9CD9F2,
    0xFF56BD19, 0x0D3D3E1A, 0x1E6DCDEE, 0xEC064EED,
    0xC38D26C4, 0x31E6A5C7, 0x22B65633, 0xD0DDD530,
    0x0417B1DB, 0xF67C32D8, 0xE52CC12C, 0x1747422F,
    0x49547E0B, 0xBB3FFD08, 0xA86F0EFC, 0x5A048DFF,
    0x8ECEE914, 0x7CA56A17, 0x6FF599E3, 0x9D9E1AE0,
    0xD3D3E1AB, 0x21B862A8, 0x32E8915C, 0xC083125F,
    0x144976B4, 0xE622F5B7, 0xF5720643, 0x07198540,
    0x590AB964, 0xAB613A67, 0xB831C993, 0x4A5A4A90,
    0x9E902E7B, 0x6CFBAD78, 0x7FAB5E8C, 0x8DC0DD8F,
    0xE330A81A, 0x115B2B19, 0x020BD8ED, 0xF0605BEE,
    0x24AA3F05, 0xD6C1BC06, 0xC5914FF2, 0x37FACCF1,
    0x69E9F0D5, 0x9B8273D6, 0x88D28022, 0x7AB90321,
    0xAE7367CA, 0x5C18E4C9, 0x4F48173D, 0xBD23943E,
    0xF36E6F75, 0x0105EC76, 0x12551F82, 0xE03E9C81,
    0x34F4F86A, 0xC69F7B69, 0xD5CF889D, 0x27A40B9E,
    0x79B737BA, 0x8BDCB4B9, 0x988C474D, 0x6AE7C44E,
    0xBE2DA0A5, 0x4C4623A6, 0x5F16D052, 0xAD7D5351
};

uint32_t crc32c(const uint8_t *data, size_t len) {
    uint32_t crc = 0xFFFFFFFF;
    while (len--) {
        crc = crc32c_table[(crc ^ *data++) & 0xFF] ^ (crc >> 8);
    }
    return crc ^ 0xFFFFFFFF;
}

// --- Initialization ---
int toe_init(uint32_t conn_table_base, uint8_t *p0_mac,
             uint8_t *p1_mac, uint32_t p0_ip, uint32_t p1_ip) {

    if (toe_initialized) return -1;

    conn_table_ddr4_base = conn_table_base;
    conn_table = (toe_conn_state_t *)conn_table_base;
    memcpy(port0_mac, p0_mac, 6);
    memcpy(port1_mac, p1_mac, 6);

    arp_table = (arp_entry_t *)(conn_table_base + (256 * sizeof(toe_conn_state_t)));
    memset(arp_table, 0, ARP_TABLE_SIZE * ARP_ENTRY_SIZE);

    // Reset TOE hardware
    toe_reg_write(TOE_REG_CTRL, TOE_CTRL_RESET);
    for (volatile int i = 0; i < 10000; i++);
    toe_reg_write(TOE_REG_CTRL, 0x00);

    // Configure MAC addresses
    uint32_t mac0_lo = port0_mac[0] | (port0_mac[1] << 8) |
                       (port0_mac[2] << 16) | (port0_mac[3] << 24);
    uint32_t mac0_hi = port0_mac[4] | (port0_mac[5] << 8);
    uint32_t mac1_lo = port1_mac[0] | (port1_mac[1] << 8) |
                       (port1_mac[2] << 16) | (port1_mac[3] << 24);
    uint32_t mac1_hi = port1_mac[4] | (port1_mac[5] << 8);

    toe_reg_write(TOE_REG_PORT0_MAC_LO, mac0_lo);
    toe_reg_write(TOE_REG_PORT0_MAC_HI, mac0_hi);
    toe_reg_write(TOE_REG_PORT1_MAC_LO, mac1_lo);
    toe_reg_write(TOE_REG_PORT1_MAC_HI, mac1_hi);

    toe_reg_write(TOE_REG_PORT0_IP, p0_ip);
    toe_reg_write(TOE_REG_PORT1_IP, p1_ip);
    toe_reg_write(TOE_REG_PORT0_NETMASK, 0x00FFFFFF);
    toe_reg_write(TOE_REG_PORT1_NETMASK, 0x00FFFFFF);
    toe_reg_write(TOE_REG_PORT0_GATEWAY, (p0_ip & 0x00FFFFFF) | 0x01000000);
    toe_reg_write(TOE_REG_PORT1_GATEWAY, (p1_ip & 0x00FFFFFF) | 0x01000000);

    toe_reg_write(TOE_REG_CONN_TABLE_BASE, conn_table_base);
    toe_reg_write(TOE_REG_ARP_TABLE_BASE,
                  conn_table_base + (256 * sizeof(toe_conn_state_t)));

    // Clear all connection states
    memset(conn_table, 0, 256 * sizeof(toe_conn_state_t));

    for (int i = 0; i < 256; i++) {
        conn_table[i].conn_id = i;
        conn_table[i].tcp_state = TCP_STATE_CLOSED;
        conn_table[i].mss = 1460;
        conn_table[i].mss_clamp = 1460;
        conn_table[i].ttl = 64;
        conn_table[i].rto = 200000;
        conn_table[i].tx_buffer_base = conn_table_base +
            (256 * sizeof(toe_conn_state_t)) + (ARP_TABLE_SIZE * ARP_ENTRY_SIZE) +
            (i * 512 * 1024);
        conn_table[i].rx_buffer_base = conn_table[i].tx_buffer_base + 262144;
    }

    uint32_t desc_base = conn_table_base + (256 * sizeof(toe_conn_state_t)) +
        (ARP_TABLE_SIZE * ARP_ENTRY_SIZE) + (256 * 512 * 1024);
    toe_reg_write(TOE_REG_TX_DESC_BASE, desc_base);
    toe_reg_write(TOE_REG_RX_DESC_BASE, desc_base + (4096 * 256));

    toe_reg_write(TOE_REG_INTERRUPT_MASK, 0x0000000F);
    toe_initialized = 1;
    return 0;
}

void toe_enable(void) {
    uint32_t ctrl = TOE_CTRL_ENABLE | TOE_CTRL_PORT0_EN | TOE_CTRL_PORT1_EN |
                    TOE_CTRL_TSO_EN | TOE_CTRL_LRO_EN | TOE_CTRL_DCTCP_EN |
                    TOE_CTRL_CRC32C_EN | TOE_CTRL_ARP_EN | TOE_CTRL_ICMP_EN;
    toe_reg_write(TOE_REG_CTRL, ctrl);
}

void toe_disable(void) {
    toe_reg_write(TOE_REG_CTRL, 0x00);
}

int toe_get_status(void) {
    return toe_reg_read(TOE_REG_STATUS);
}

// --- Connection Management ---
int toe_connect(uint8_t conn_id, uint32_t dst_ip, uint16_t dst_port,
                uint16_t src_port, uint8_t port_id) {

    if (!toe_initialized || conn_id >= 256) return -1;

    toe_conn_state_t *conn = &conn_table[conn_id];
    if (conn->tcp_state != TCP_STATE_CLOSED) return -2;

    conn->dst_ip = dst_ip;
    conn->src_ip = (port_id == 0) ?
        toe_reg_read(TOE_REG_PORT0_IP) : toe_reg_read(TOE_REG_PORT1_IP);
    conn->dst_port = dst_port;
    conn->src_port = src_port;
    conn->port_id = port_id;
    conn->tcp_state = TCP_STATE_SYN_SENT;
    conn->snd_nxt = 0xA0000000 + (conn_id * 0x10000);
    conn->snd_una = conn->snd_nxt;
    conn->snd_wnd = 65535;
    conn->snd_cwnd = conn->mss * 10;
    conn->snd_ssthresh = 65535;
    conn->rcv_nxt = 0;
    conn->rcv_wnd = 262144;
    conn->rto = 200000;
    conn->srtt = 0;
    conn->rttvar = 0;
    conn->retransmit_count = 0;
    conn->dctcp_ecn_flags = 0;
    conn->tx_buffer_head = 0;
    conn->tx_buffer_tail = 0;
    conn->rx_buffer_head = 0;
    conn->rx_buffer_tail = 0;

    uint32_t cmd = (TOE_CMD_CONNECT << 24) | (conn_id << 16) | (port_id << 8);
    toe_reg_write(TOE_REG_CMD_QUEUE, cmd);
    return 0;
}

int toe_disconnect(uint8_t conn_id) {
    if (!toe_initialized || conn_id >= 256) return -1;

    toe_conn_state_t *conn = &conn_table[conn_id];
    if (conn->tcp_state == TCP_STATE_CLOSED ||
        conn->tcp_state == TCP_STATE_TIME_WAIT) return -2;

    uint32_t cmd = (TOE_CMD_DISCONNECT << 24) | (conn_id << 16);
    toe_reg_write(TOE_REG_CMD_QUEUE, cmd);
    return 0;
}

int toe_get_conn_state(uint8_t conn_id, toe_conn_state_t *state) {
    if (!toe_initialized || conn_id >= 256 || state == NULL) return -1;
    memcpy(state, &conn_table[conn_id], sizeof(toe_conn_state_t));
    uint32_t computed_crc = crc32c((uint8_t *)state, sizeof(toe_conn_state_t) - 4);
    if (computed_crc != state->checksum) return -3;
    return 0;
}

// --- Data Transfer ---
int toe_send_data(uint8_t conn_id, const uint8_t *data, uint32_t len) {
    if (!toe_initialized || conn_id >= 256 || data == NULL || len == 0) return -1;

    toe_conn_state_t *conn = &conn_table[conn_id];
    if (conn->tcp_state != TCP_STATE_ESTABLISHED) return -2;

    uint32_t tx_buf_size = 262144;
    uint16_t head = conn->tx_buffer_head;
    uint16_t tail = conn->tx_buffer_tail;
    uint32_t used = (head >= tail) ? (head - tail) : (tx_buf_size - tail + head);
    uint32_t available = tx_buf_size - used - 1;
    if (len > available) return -3;

    uint8_t *tx_buf = (uint8_t *)conn->tx_buffer_base;
    uint32_t first_chunk = tx_buf_size - tail;
    if (len <= first_chunk) {
        memcpy(tx_buf + tail, data, len);
    } else {
        memcpy(tx_buf + tail, data, first_chunk);
        memcpy(tx_buf, data + first_chunk, len - first_chunk);
    }
    conn->tx_buffer_tail = (tail + len) % tx_buf_size;

    uint32_t cmd = (TOE_CMD_SEND << 24) | (conn_id << 16) | (len & 0xFFFF);
    toe_reg_write(TOE_REG_CMD_QUEUE, cmd);
    return (int)len;
}

int toe_recv_data(uint8_t conn_id, uint8_t *buffer, uint32_t max_len,
                  uint32_t *actual_len) {
    if (!toe_initialized || conn_id >= 256 || buffer == NULL ||
        max_len == 0 || actual_len == NULL) return -1;

    toe_conn_state_t *conn = &conn_table[conn_id];
    if (conn->tcp_state != TCP_STATE_ESTABLISHED) return -2;

    uint32_t rx_buf_size = 262144;
    uint16_t head = conn->rx_buffer_head;
    uint16_t tail = conn->rx_buffer_tail;
    uint32_t available = (tail >= head) ? (tail - head) : (rx_buf_size - head + tail);
    if (available == 0) { *actual_len = 0; return 0; }

    uint32_t read_len = (available < max_len) ? available : max_len;
    uint8_t *rx_buf = (uint8_t *)conn->rx_buffer_base;
    uint32_t first_chunk = rx_buf_size - head;
    if (read_len <= first_chunk) {
        memcpy(buffer, rx_buf + head, read_len);
    } else {
        memcpy(buffer, rx_buf + head, first_chunk);
        memcpy(buffer + first_chunk, rx_buf, read_len - first_chunk);
    }
    conn->rx_buffer_head = (head + read_len) % rx_buf_size;
    *actual_len = read_len;
    return 0;
}

// --- Statistics ---
void toe_get_stats(uint64_t *tx_pkts, uint64_t *rx_pkts,
                   uint64_t *tx_bytes, uint64_t *rx_bytes,
                   uint64_t *tx_drop, uint64_t *rx_drop,
                   uint64_t *retx_count) {
    uint32_t stats_base = toe_reg_read(TOE_REG_STATS_BASE);
    volatile uint64_t *stats = (volatile uint64_t *)stats_base;
    if (tx_pkts)   *tx_pkts   = stats[0];
    if (rx_pkts)   *rx_pkts   = stats[1];
    if (tx_bytes)  *tx_bytes  = stats[2];
    if (rx_bytes)  *rx_bytes  = stats[3];
    if (tx_drop)   *tx_drop   = stats[4];
    if (rx_drop)   *rx_drop   = stats[5];
    if (retx_count) *retx_count = stats[6];
}

void toe_set_interrupt_mask(uint32_t mask) {
    toe_reg_write(TOE_REG_INTERRUPT_MASK, mask);
}

// --- Error Handler ---
static void toe_error_handler(uint8_t conn_id, uint8_t error_code) {
    toe_conn_state_t *conn = &conn_table[conn_id];
    switch (error_code) {
        case 0x01: conn->tcp_state = TCP_STATE_CLOSED; conn->retransmit_count = 0; break;
        case 0x02:
            toe_reg_write(TOE_REG_RST_QUEUE, (TOE_CMD_RST << 24) | (conn_id << 16));
            conn->tcp_state = TCP_STATE_CLOSED; break;
        case 0x03: conn->total_rx_bytes++; break;
        case 0x04: conn->tcp_state = TCP_STATE_CLOSED; break;
        default: break;
    }
}

// Weak callbacks for NVMe-oF integration
__attribute__((weak)) void nvmeof_conn_ready_callback(uint8_t conn_id) { (void)conn_id; }
__attribute__((weak)) void nvmeof_conn_closed_callback(uint8_t conn_id) { (void)conn_id; }
__attribute__((weak)) void nvmeof_data_ready_callback(uint8_t conn_id) { (void)conn_id; }
