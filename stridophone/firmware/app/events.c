/*
 * events.c — event queue + JSON serialisation + SD persistence.
 *
 * Author : jayis1
 * License: MIT
 */
#include "events.h"
#include "../drivers/sd.h"
#include "../board.h"
#include <stdio.h>
#include <string.h>

static event_t g_queue[EVENT_QUEUE_DEPTH];
static int     g_head = 0, g_tail = 0;
static uint32_t g_next_id = 1;
static uint32_t g_boot_ms = 0;

extern uint32_t tick_ms_get(void);  /* weak, defined in main.c as tick_ms */
__attribute__((weak)) uint32_t tick_ms_get(void) { return 0; }

void events_init(void) {
    g_head = g_tail = 0;
    g_next_id = 1;
    g_boot_ms = tick_ms_get();
    sd_log_open("strido.log");
}

void event_build(event_t *e, const clf_result_t *clf,
                 const uint32_t *doa_hist, int nbins,
                 float temp_c, float rh_pct) {
    e->id           = g_next_id++;
    e->timestamp_ms = tick_ms_get() - g_boot_ms;
    e->species_id   = clf->species_id;
    e->confidence   = clf->confidence;
    /* Pick the dominant DOA bin from the histogram. */
    int best = 0;
    for (int i = 1; i < nbins; i++) if (doa_hist[i] > doa_hist[best]) best = i;
    e->doa_bin      = best;
    e->temperature  = temp_c;
    e->humidity     = rh_pct;
    e->pulse_rate   = 0.0f;       /* filled from feature vector in caller */
    e->vibe_crest   = 0.0f;
    e->vibe_kurtosis= 0.0f;
}

int events_push(const event_t *e) {
    int next = (g_tail + 1) % EVENT_QUEUE_DEPTH;
    if (next == g_head) {
        /* Overflow: drop oldest. */
        g_head = (g_head + 1) % EVENT_QUEUE_DEPTH;
    }
    g_queue[g_tail] = *e;
    g_tail = next;
    /* Also append a one-line JSON record to SD. */
    char line[160];
    int n = snprintf(line, sizeof(line),
        "{\"id\":%lu,\"t\":%lu,\"sp\":%d,\"conf\":%d,\"az\":%d,"
        "\"T\":%.1f,\"RH\":%.1f}\n",
        (unsigned long)e->id, (unsigned long)e->timestamp_ms,
        e->species_id, e->confidence, e->doa_bin,
        e->temperature, e->humidity);
    sd_log_append(line);
    return 0;
}

int events_flush_sd(void) {
    return sd_log_close();   /* close+reopen to flush */
}

int events_pop_to_json(char *buf, int maxlen, int max_events) {
    int off = 0;
    int n = 0;
    while (g_head != g_tail && n < max_events) {
        event_t *e = &g_queue[g_head];
        int w = snprintf(buf + off, maxlen - off,
            "%s{\"id\":%lu,\"sp\":%d,\"conf\":%d,\"az\":%d}",
            (n == 0) ? "[" : ",",
            (unsigned long)e->id, e->species_id, e->confidence, e->doa_bin);
        if (w < 0 || off + w >= maxlen - 2) break;
        off += w;
        g_head = (g_head + 1) % EVENT_QUEUE_DEPTH;
        n++;
    }
    if (n > 0) {
        buf[off++] = ']';
        buf[off]   = '\0';
    } else {
        buf[0] = '['; buf[1] = ']'; buf[2] = '\0';
    }
    return n;
}