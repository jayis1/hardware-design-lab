// toe_driver.h — TCP Offload Engine Driver for Aether-Link
// Manages 256 hardware TCP connections with TSO/LRO/DCTCP support.
// This header defines the public API for the TOE hardware block.

#ifndef TOE_DRIVER_H
#define TOE_DRIVER_H

#include <stdint.h>
#include <stddef.h>

// --- Connection State Structure ---
// Each connection occupies 4096 bytes in DDR4 for alignment.
// 256 connections × 4096 = 1 MB total connection table.

typedef struct {
    uint32_t src_ip;           // Source IP address (host side)
    uint32_t dst_ip;           // Destination IP address (target side)
    uint16_t src_port;         // Source TCP port
    uint16_t dst_port;         // Destination TCP port
    uint32_t snd_una;          // Oldest unacknowledged sequence number
    uint32_t snd_nxt;          // Next sequence number to send
    uint32_t snd_wnd;          // Send window (from target)
    uint32_t snd_wl1;          // Sequence number of last window update
    uint32_t snd_wl2;          // Ack number of last window update
    uint32_t snd_cwnd;         // Congestion window (DCTCP)
    uint32_t snd_ssthresh;     // Slow start threshold
    uint32_t rcv_nxt;          // Next expected receive sequence number
    uint32_t rcv_wnd;          // Receive window (advertised to target)
    uint32_t rcv_adv;          // Last advertised receive window
    uint16_t mss;              // Maximum segment size
    uint16_t mss_clamp;        // Clamped MSS from path MTU discovery
    uint8_t  tcp_state;        // TCP FSM state
    uint8_t  retransmit_count; // Number of retransmissions for current segment
    uint8_t  dctcp_ecn_flags;  // DCTCP ECN state
    uint8_t  ttl;              // IP TTL value
    uint32_t rto;              // Retransmission timeout (microseconds)
    uint32_t srtt;             // Smoothed round-trip time
    uint32_t rttvar;           // Round-trip time variation
    uint32_t rtx_timer;        // Retransmit timer (hardware countdown)
    uint32_t keepalive_timer;  // Keepalive timer
    uint32_t ts_recent;        // Most recent timestamp from peer
    uint32_t ts_val;           // Current timestamp value
    uint32_t last_ack_sent;    // Last ACK sent to peer
    uint32_t total_tx_bytes;   // Total bytes transmitted
    uint32_t total_rx_bytes;   // Total bytes received
    uint32_t total_retx;       // Total retransmissions
    uint32_t tx_buffer_base;   // DDR4 base address of TX buffer (256KB)
    uint32_t rx_buffer_base;   // DDR4 base address of RX buffer (256KB)
    uint16_t tx_buffer_head;   // TX ring buffer head offset
    uint16_t tx_buffer_tail;   // TX ring buffer tail offset
    uint16_t rx_buffer_head;   // RX ring buffer head offset
    uint16_t rx_buffer_tail;   // RX ring buffer tail offset
    uint8_t  port_id;          // QSFP28 port (0 or 1)
    uint8_t  conn_id;          // Connection ID (0-255)
    uint8_t  nvme_qid;         // Associated NVMe queue ID
    uint8_t  padding[3];       // Align to 4-byte boundary
    uint32_t checksum;         // CRC32 of connection state (for integrity)
} toe_conn_state_t;

// --- TCP States ---
#define TCP_STATE_CLOSED       0
#define TCP_STATE_LISTEN       1
#define TCP_STATE_SYN_SENT     2
#define TCP_STATE_SYN_RCVD     3
#define TCP_STATE_ESTABLISHED  4
#define TCP_STATE_FIN_WAIT1    5
#define TCP_STATE_FIN_WAIT2    6
#define TCP_STATE_CLOSE_WAIT   7
#define TCP_STATE_CLOSING      8
#define TCP_STATE_LAST_ACK     9
#define TCP_STATE_TIME_WAIT    10

// --- Function Prototypes ---

// Initialization
int  toe_init(uint32_t conn_table_ddr4_base, uint8_t *port0_mac,
              uint8_t *port1_mac, uint32_t port0_ip, uint32_t port1_ip);
void toe_enable(void);
void toe_disable(void);
int  toe_get_status(void);

// Connection management
int  toe_connect(uint8_t conn_id, uint32_t dst_ip, uint16_t dst_port,
                 uint16_t src_port, uint8_t port_id);
int  toe_disconnect(uint8_t conn_id);
int  toe_get_conn_state(uint8_t conn_id, toe_conn_state_t *state);

// Data transfer
int  toe_send_data(uint8_t conn_id, const uint8_t *data, uint32_t len);
int  toe_recv_data(uint8_t conn_id, uint8_t *buffer, uint32_t max_len,
                   uint32_t *actual_len);

// Statistics
void toe_get_stats(uint64_t *tx_pkts, uint64_t *rx_pkts,
                   uint64_t *tx_bytes, uint64_t *rx_bytes,
                   uint64_t *tx_drop, uint64_t *rx_drop,
                   uint64_t *retx_count);

// Interrupt handling
void toe_interrupt_handler(void);
void toe_set_interrupt_mask(uint32_t mask);

// CRC32C utility
uint32_t crc32c(const uint8_t *data, size_t len);

#endif // TOE_DRIVER_H
