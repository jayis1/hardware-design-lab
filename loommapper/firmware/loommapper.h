/* LoomMapper public firmware interfaces
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: MIT
 */
#ifndef LOOMMAPPER_H
#define LOOMMAPPER_H
#include "board.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
typedef enum { LM_OK=0, LM_ERR_ARG=-1, LM_ERR_IO=-2, LM_ERR_TIMEOUT=-3, LM_ERR_CRC=-4, LM_ERR_STATE=-5 } lm_result_t;
typedef enum { LM_IDLE, LM_BASELINE, LM_SCAN, LM_FLEX_WATCH, LM_REPORT, LM_FAULT } lm_state_t;
typedef enum { LM_EDGE_UNKNOWN, LM_EDGE_GOOD, LM_EDGE_OPEN, LM_EDGE_SHORT, LM_EDGE_INTERMITTENT, LM_EDGE_CROSSTALK } lm_edge_kind_t;
typedef struct {
    uint8_t channel;
    bool clip_present;
    int32_t in_phase_uv;
    int32_t quadrature_uv;
    int32_t baseline_uv;
    uint16_t noise_uv;
} lm_channel_sample_t;
typedef struct {
    uint8_t source;
    uint8_t sink;
    lm_edge_kind_t kind;
    uint16_t confidence_permille;
    int32_t magnitude_uv;
    uint16_t variation_permille;
    uint32_t first_seen_ms;
    uint32_t last_seen_ms;
} lm_edge_t;
typedef struct {
    lm_edge_t edges[LM_MAX_EDGES];
    uint16_t edge_count;
    uint32_t scan_sequence;
    uint32_t graph_crc;
    bool overflow;
} lm_graph_t;
typedef struct {
    uint32_t timestamp_ms;
    uint8_t source;
    uint8_t sink;
    lm_edge_kind_t before;
    lm_edge_kind_t after;
    uint16_t severity;
} lm_event_t;
typedef struct {
    lm_event_t entries[LM_EVENT_CAPACITY];
    uint16_t head;
    uint16_t count;
} lm_event_log_t;
typedef struct {
    lm_state_t state;
    uint8_t active_source;
    uint16_t settled_scans;
    uint16_t fault_count;
    uint32_t next_scan_ms;
    uint32_t next_report_ms;
    uint16_t battery_mv;
} lm_context_t;
typedef struct {
    int32_t offset_uv[LM_CHANNELS];
    uint16_t gain_permille[LM_CHANNELS];
    uint16_t noise_limit_uv[LM_CHANNELS];
    uint32_t valid_mask;
    uint32_t crc;
} lm_calibration_block_t;
typedef struct {
    int32_t minimum;
    int32_t maximum;
    int64_t sum;
    uint64_t sum_squares;
    uint32_t count;
    uint32_t clipped;
} lm_signal_stats_t;
typedef struct {
    uint16_t good;
    uint16_t open;
    uint16_t shorted;
    uint16_t intermittent;
    uint16_t crosstalk;
    uint16_t unknown;
} lm_graph_summary_t;
void platform_init(void);
uint32_t platform_millis(void);
void platform_advance(uint32_t ms);
void platform_sleep(void);
void platform_set_led(bool on);
int platform_receive(uint8_t *data,size_t capacity);
void platform_notify(const uint8_t *data,size_t length);
lm_result_t driver_init(void);
lm_result_t driver_select_source(uint8_t source);
lm_result_t driver_capture(uint8_t source,lm_channel_sample_t out[LM_CHANNELS]);
uint16_t driver_battery_mv(void);
void driver_set_flex(bool enabled);
void graph_init(lm_graph_t *graph);
lm_edge_kind_t classify_edge(const lm_channel_sample_t *sample,bool self_channel);
uint16_t edge_confidence(const lm_channel_sample_t *sample,lm_edge_kind_t kind);
lm_result_t graph_update(lm_graph_t *graph,uint8_t source,const lm_channel_sample_t samples[LM_CHANNELS],uint32_t now,lm_event_log_t *events);
const lm_edge_t *graph_find(const lm_graph_t *graph,uint8_t source,uint8_t sink);
void event_log_init(lm_event_log_t *log);
void event_log_append(lm_event_log_t *log,const lm_event_t *event);
bool event_log_get(const lm_event_log_t *log,uint16_t newest_index,lm_event_t *event);
uint16_t lm_crc16(const uint8_t *data,size_t length);
uint32_t lm_crc32(const uint8_t *data,size_t length);
size_t protocol_encode_status(const lm_context_t *ctx,const lm_graph_t *graph,uint8_t *out,size_t capacity);
size_t protocol_encode_edges(const lm_graph_t *graph,uint8_t start,uint8_t *out,size_t capacity);
lm_result_t protocol_handle(const uint8_t *packet,size_t length,lm_context_t *ctx,lm_graph_t *graph,lm_event_log_t *events);
void lm_calibration_defaults(lm_calibration_block_t *block);
bool lm_calibration_valid(const lm_calibration_block_t *block);
lm_result_t lm_calibration_set_offset(lm_calibration_block_t *block,uint8_t channel,int32_t offset_uv);
lm_result_t lm_calibration_set_gain(lm_calibration_block_t *block,uint8_t channel,uint16_t gain_permille);
int32_t lm_calibration_apply(const lm_calibration_block_t *block,uint8_t channel,int32_t raw_uv);
void lm_stats_init(lm_signal_stats_t *stats);
void lm_stats_push(lm_signal_stats_t *stats,int32_t sample);
int32_t lm_stats_mean(const lm_signal_stats_t *stats);
uint32_t lm_stats_rms(const lm_signal_stats_t *stats);
uint32_t lm_stats_peak_to_peak(const lm_signal_stats_t *stats);
uint16_t lm_signal_quality(const lm_signal_stats_t *signal,const lm_signal_stats_t *noise);
uint16_t lm_variation_permille(int32_t baseline,int32_t observed);
void lm_graph_summarize(const lm_graph_t *graph,lm_graph_summary_t *summary);
uint16_t lm_graph_health(const lm_graph_t *graph);
uint16_t lm_graph_confidence(const lm_graph_t *graph);
const lm_edge_t *lm_graph_most_variable(const lm_graph_t *graph);
bool lm_graph_has_path(const lm_graph_t *graph,uint8_t source,uint8_t sink);
uint16_t lm_event_severity_max(const lm_event_log_t *log);
uint32_t lm_event_recent_count(const lm_event_log_t *log,uint32_t now,uint32_t window_ms);
uint16_t lm_battery_percent(uint16_t millivolts);
bool lm_battery_allows_scan(uint16_t millivolts,lm_state_t requested_state);
uint32_t lm_estimated_runtime_minutes(uint16_t battery_mv,uint16_t capacity_mah,lm_state_t state);
uint16_t lm_scan_progress(uint8_t active_source,uint16_t settled_scans,uint16_t required_scans);
#endif
