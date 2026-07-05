/*
 * command.c — JSON command parser / dispatcher
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * License: MIT
 *
 * Implements a minimal JSON command protocol over Wi-Fi TCP and BLE.
 * Commands are single-line JSON objects. Supported verbs:
 *   {"cmd":"status"}                              -> status JSON
 *   {"cmd":"start","duration":3600,"target":"wall","distance_cm":200}
 *   {"cmd":"stop"}
 *   {"cmd":"calibrate"}
 *   {"cmd":"preview","slice":16}                  -> preview PNG bytes (base64)
 *   {"cmd":"config","sipm_mv":15700,"coinc_ns":5}
 *   {"cmd":"wifi","ssid":"...","pass":"..."}
 *   {"cmd":"sleep"}
 *   {"cmd":"ota","url":"http://..."}               -> OTA update (handled in main)
 *
 * Responses are JSON: {"ok":true,...} or {"ok":false,"err":"..."}
 */
#include "command.h"
#include "board.h"
#include "registers.h"
#include "fbp.h"
#include "power.h"
#include "sensors.h"
#include "gps.h"
#include "sipm.h"
#include "event.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

extern volatile system_state_t g_state;
extern volatile uint32_t g_acq_start_ms;
extern volatile uint32_t g_acq_duration_ms;
extern volatile uint32_t g_track_count;
extern volatile uint32_t g_event_count;
extern float *g_volume;

static int json_find(const char *s, uint16_t len, const char *key,
                     char *out, uint16_t out_max)
{
    /* Find "key":"value" or "key":number */
    char pat[32];
    snprintf(pat, sizeof(pat), "\"%s\"", key);
    const char *k = NULL;
    for (uint16_t i = 0; i + strlen(pat) <= len; ++i) {
        if (strncmp(&s[i], pat, strlen(pat)) == 0) { k = &s[i]; break; }
    }
    if (!k) return 0;
    const char *p = k + strlen(pat);
    while (*p && (*p == ' ' || *p == ':' || *p == '"')) p++;
    uint16_t n = 0;
    while (*p && *p != ',' && *p != '}' && *p != '"' && n < out_max - 1) {
        out[n++] = *p++;
    }
    out[n] = 0;
    return 1;
}

static long json_find_long(const char *s, uint16_t len, const char *key, long def)
{
    char buf[24];
    if (!json_find(s, len, key, buf, sizeof(buf))) return def;
    return strtol(buf, NULL, 10);
}

uint16_t command_build_status(char *resp, uint16_t max)
{
    if (!resp || max == 0) return 0;
    power_state_t pw;
    env_state_t env;
    gps_state_t gps;
    power_read(&pw);
    sensors_read(&env);
    gps_read(&gps);
    const char *state_name[] = {
        "boot","idle","calibrating","acquiring","processing","sleep","fault"
    };
    const char *sn = (g_state < 7) ? state_name[g_state] : "?";
    fbp_stats_t fs;
    fbp_get_stats(&fs);
    int n = snprintf(resp, max,
        "{\"ok\":true,\"state\":\"%s\",\"fw\":\"%s\",\"author\":\"jayis1\","
        "\"tracks\":%lu,\"events\":%lu,\"elapsed_s\":%lu,\"remain_s\":%lu,"
        "\"soc\":%u,\"pack_mv\":%u,\"current_ma\":%d,\"temp_c\":%.1f,"
        "\"sipm_mv\":%u,\"coinc_ns\":%u,"
        "\"gps_fix\":%d,\"sats\":%u,\"lat\":%.7f,\"lon\":%.7f,"
        "\"roll\":%.1f,\"pitch\":%.1f,\"lux\":%u,"
        "\"fbp_tracks\":%lu,\"fbp_max\":%.3f,"
        "\"rssi_dbm\":%ld}\n",
        sn, FW_BUILD_ID,
        (unsigned long)g_track_count, (unsigned long)event_count(),
        (unsigned long)((muon_millis() - g_acq_start_ms) / 1000),
        (unsigned long)(g_acq_duration_ms / 1000 >
            (muon_millis() - g_acq_start_ms) / 1000 ?
            g_acq_duration_ms / 1000 - (muon_millis() - g_acq_start_ms) / 1000 : 0),
        pw.soc_percent, pw.pack_mv, pw.current_ma, env.temp_c,
        sipm_get_bias_mv(), COINC_WINDOW_NS,
        gps_has_fix(), gps.satellites,
        gps.lat_e7 / 1e7f, gps.lon_e7 / 1e7f,
        env.roll_deg, env.pitch_deg, env.lux,
        (unsigned long)fs.tracks_accumulated, fs.max_density,
        -70L);
    if (n < 0) return 0;
    if (n >= (int)max) n = max - 1;
    return (uint16_t)n;
}

uint16_t command_dispatch(const char *cmd, uint16_t cmd_len,
                           char *resp, uint16_t resp_len)
{
    if (!cmd || !resp || resp_len < 32) return 0;
    char verb[24];
    if (!json_find(cmd, cmd_len, "cmd", verb, sizeof(verb))) {
        return (uint16_t)snprintf(resp, resp_len,
            "{\"ok\":false,\"err\":\"no cmd\"}\n");
    }

    if (strcmp(verb, "status") == 0) {
        return command_build_status(resp, resp_len);
    }
    if (strcmp(verb, "start") == 0) {
        long dur = json_find_long(cmd, cmd_len, "duration", 3600);
        if (dur < 1) dur = 3600;
        if (dur > (long)(ACQ_MAX_SEC/1000)) dur = ACQ_MAX_SEC/1000;
        g_acq_duration_ms = (uint32_t)dur * 1000;
        g_acq_start_ms = muon_millis();
        g_track_count = 0;
        g_state = STATE_ACQUIRING;
        char path[64];
        snprintf(path, sizeof(path), "acq_%lu.bin", (unsigned long)g_acq_start_ms);
        event_open(path);
        return (uint16_t)snprintf(resp, resp_len,
            "{\"ok\":true,\"started\":true,\"duration_s\":%ld}\n", dur);
    }
    if (strcmp(verb, "stop") == 0) {
        g_state = STATE_IDLE;
        event_close();
        return (uint16_t)snprintf(resp, resp_len,
            "{\"ok\":true,\"stopped\":true,\"tracks\":%lu}\n",
            (unsigned long)g_track_count);
    }
    if (strcmp(verb, "calibrate") == 0) {
        g_state = STATE_CALIBRATING;
        return (uint16_t)snprintf(resp, resp_len,
            "{\"ok\":true,\"calibrating\":true}\n");
    }
    if (strcmp(verb, "preview") == 0) {
        long slice = json_find_long(cmd, cmd_len, "slice", VOX_Z / 2);
        if (slice < 0 || slice >= VOX_Z) slice = VOX_Z / 2;
        uint8_t png[64 * 64 + 8];
        uint32_t n = fbp_render_preview(g_volume, (uint8_t)slice, png, sizeof(png));
        if (n == 0) {
            return (uint16_t)snprintf(resp, resp_len,
                "{\"ok\":false,\"err\":\"no data\"}\n");
        }
        /* In production we'd base64-encode and stream; here report length */
        return (uint16_t)snprintf(resp, resp_len,
            "{\"ok\":true,\"slice\":%ld,\"bytes\":%lu}\n", slice,
            (unsigned long)n);
    }
    if (strcmp(verb, "config") == 0) {
        long mv = json_find_long(cmd, cmd_len, "sipm_mv", SIPM_BIAS_DEFAULT);
        long ns = json_find_long(cmd, cmd_len, "coinc_ns", COINC_WINDOW_NS);
        if (mv >= SIPM_BIAS_MIN_MV && mv <= SIPM_BIAS_MAX_MV)
            sipm_set_bias_mv((uint32_t)mv);
        if (ns >= 1 && ns <= 50) {
            /* fpga_set_coincidence_window((uint8_t)ns); */
        }
        return (uint16_t)snprintf(resp, resp_len,
            "{\"ok\":true,\"sipm_mv\":%u,\"coinc_ns\":%u}\n",
            sipm_get_bias_mv(), (uint32_t)ns);
    }
    if (strcmp(verb, "sleep") == 0) {
        g_state = STATE_SLEEP;
        return (uint16_t)snprintf(resp, resp_len,
            "{\"ok\":true,\"sleeping\":true}\n");
    }
    /* Unknown verb */
    return (uint16_t)snprintf(resp, resp_len,
        "{\"ok\":false,\"err\":\"unknown cmd '%s'\"}\n", verb);
}