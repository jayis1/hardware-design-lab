/*
 * storage.c — Data Logging Storage (SD card + flash ring buffer)
 *
 * Logs all sensor samples to an SD card (if present) as CSV files,
 * with a flash-based ring buffer fallback when no SD card is mounted.
 * Each batch gets its own CSV file with a header row and timestamped
 * sample rows.
 *
 * CSV format:
 *   timestamp,phase,cell_density,z_mag_1k,z_mag_10k,z_mag_100k,
 *   co2_ppm,cer_mmol_lh,ph,ph_rate,temp_c,ambient_c,rh_pct,
 *   bubble_rate,abv,attenuation,spoilage_risk,health_score,battery_soc
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: GPL-3.0
 */

#include "storage.h"
#include "../board.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"
#include "driver/sdspi_host.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

static const char *TAG = "storage";

static bool s_sd_mounted = false;
static char s_current_filename[64] = {0};
static uint32_t s_batch_samples = 0;
static uint32_t s_total_samples = 0;

/* Flash ring buffer fallback (using NVS partition) */
#define FLASH_RING_SIZE  4096
static uint8_t s_flash_ring[FLASH_RING_SIZE];
static int s_flash_ring_head = 0;
static bool s_flash_ring_used = false;

/* ------------------------------------------------------------------- */
/* Initialize SD card                                                  */
/* ------------------------------------------------------------------- */
int storage_init(void)
{
    /* Try to mount SD card via SPI */
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = SPI2_HOST;
    host.max_freq_khz = 20000;

    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = SD_CS;
    slot_config.host_id = SPI2_HOST;

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024,
    };

    sdmmc_card_t *card;
    esp_err_t err = esp_vfs_fat_sdspi_mount(SD_MOUNT_POINT, &host,
                                             &slot_config, &mount_config, &card);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "SD card mount failed: %s — using flash ring buffer",
                 esp_err_to_name(err));
        s_sd_mounted = false;
        s_flash_ring_used = true;
        memset(s_flash_ring, 0, FLASH_RING_SIZE);
        return 0;
    }

    s_sd_mounted = true;
    ESP_LOGI(TAG, "SD card mounted at %s", SD_MOUNT_POINT);
    return 0;
}

/* ------------------------------------------------------------------- */
/* Create new batch file with CSV header                               */
/* ------------------------------------------------------------------- */
static void create_batch_file(const char *batch_name)
{
    if (!s_sd_mounted)
        return;

    /* Generate filename from batch name + timestamp */
    uint64_t now = esp_timer_get_time() / 1000000;  /* seconds */
    snprintf(s_current_filename, sizeof(s_current_filename),
             "%s/batch_%lu_%s.csv", SD_MOUNT_POINT,
             (unsigned long)now, batch_name);

    /* Replace spaces in filename */
    for (char *p = s_current_filename; *p; p++)
        if (*p == ' ') *p = '_';

    FILE *f = fopen(s_current_filename, "w");
    if (f) {
        fprintf(f, "# FermenTiq Batch Log\n");
        fprintf(f, "# Author: jayis1\n");
        fprintf(f, "# Batch: %s\n", batch_name);
        fprintf(f, "# Created: %lu\n", (unsigned long)now);
        fprintf(f, "timestamp_ms,phase,cell_density,z_mag_1k,z_mag_10k,"
                "z_mag_100k,cole_alpha,co2_ppm,cer_mmol_lh,dissolved_g_l,"
                "ph,ph_rate,temp_c,ambient_c,rh_pct,dew_point_c,"
                "bubble_rate,spectral_centroid,abv,attenuation,"
                "spoilage_risk,health_score,batch_age_h,battery_soc,"
                "battery_mv,usb_connected\n");
        fclose(f);
        s_batch_samples = 0;
        ESP_LOGI(TAG, "Created batch file: %s", s_current_filename);
    } else {
        ESP_LOGE(TAG, "Failed to create batch file: %s", s_current_filename);
    }
}

/* ------------------------------------------------------------------- */
/* Log a sample to SD card or flash ring buffer                       */
/* ------------------------------------------------------------------- */
int storage_log_sample(const fermentiq_state_t *state)
{
    if (!state || !state->active)
        return 0;

    uint64_t now = esp_timer_get_time() / 1000;

    /* Create new file if batch just started */
    if (s_sd_mounted && s_current_filename[0] == '\0') {
        create_batch_file(state->batch_name);
    }

    /* Format CSV row */
    char row[512];
    int len = snprintf(row, sizeof(row),
        "%llu,%s,%.4e,%.1f,%.1f,%.1f,%.3f,%u,%.4f,%.3f,"
        "%.3f,%.5f,%.3f,%.1f,%.1f,%.1f,%.2f,%.0f,%.3f,%.1f,"
        "%d,%.0f,%lu,%.0f,%.0f,%s\n",
        (unsigned long long)now,
        phase_names[state->fusion.phase],
        state->impedance.valid ? state->impedance.cell_density : 0.0f,
        state->impedance.z_mag_1k,
        state->impedance.z_mag_10k,
        state->impedance.z_mag_100k,
        state->impedance.cole_alpha,
        state->co2.valid ? state->co2.co2_ppm : 0,
        state->co2.cer_mmol_lh,
        state->co2.co2_dissolved,
        state->ph.valid ? state->ph.ph : 0.0f,
        state->ph.ph_rate,
        state->temp.valid ? state->temp.temp_c : 0.0f,
        state->temp.ambient_temp_c,
        state->temp.ambient_rh,
        state->temp.dew_point_c,
        state->acoustic.bubble_rate,
        state->acoustic.spectral_centroid,
        state->fusion.abv_estimate,
        state->fusion.attenuation,
        state->fusion.spoilage_risk,
        state->fusion.health_score,
        (unsigned long)state->fusion.batch_age_hours,
        state->battery_soc,
        state->battery_mv,
        state->usb_connected ? "1" : "0"
    );

    if (s_sd_mounted && s_current_filename[0] != '\0') {
        FILE *f = fopen(s_current_filename, "a");
        if (f) {
            fputs(row, f);
            fclose(f);
            s_batch_samples++;
            s_total_samples++;
        } else {
            ESP_LOGW(TAG, "Failed to append to batch file");
            /* Fallback to flash ring */
            s_flash_ring_used = true;
        }
    }

    if (s_flash_ring_used) {
        /* Store in flash ring buffer (simplified — just record that we
         * would store here; a full implementation would use a wear-leveled
         * flash partition) */
        int remaining = FLASH_RING_SIZE - s_flash_ring_head;
        if (len < remaining) {
            memcpy(&s_flash_ring[s_flash_ring_head], row, len);
            s_flash_ring_head += len;
        } else {
            /* Wrap around */
            memcpy(&s_flash_ring[0], row, len);
            s_flash_ring_head = len;
        }
        s_total_samples++;
    }

    return 0;
}

/* ------------------------------------------------------------------- */
/* Export batch data to a specified file path                         */
/* ------------------------------------------------------------------- */
int storage_export_batch(const char *filepath)
{
    if (!s_sd_mounted || !filepath)
        return -1;

    /* Copy current batch file to the specified path */
    FILE *src = fopen(s_current_filename, "r");
    if (!src) return -1;

    FILE *dst = fopen(filepath, "w");
    if (!dst) {
        fclose(src);
        return -1;
    }

    char buf[512];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), src)) > 0)
        fwrite(buf, 1, n, dst);

    fclose(src);
    fclose(dst);
    ESP_LOGI(TAG, "Exported batch to %s", filepath);
    return 0;
}

/* ------------------------------------------------------------------- */
/* Get logging statistics                                             */
/* ------------------------------------------------------------------- */
int storage_get_stats(uint32_t *total_samples, uint32_t *batch_samples)
{
    if (total_samples) *total_samples = s_total_samples;
    if (batch_samples) *batch_samples = s_batch_samples;
    return 0;
}