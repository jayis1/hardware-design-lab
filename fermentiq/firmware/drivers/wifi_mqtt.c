/*
 * wifi_mqtt.c — WiFi + MQTT Client for Home Assistant Integration
 *
 * Connects to the configured WiFi network and MQTT broker, publishes
 * sensor state as JSON to the fermentiq/<device_id>/ topic tree, and
 * supports Home Assistant MQTT auto-discovery.
 *
 * Published topics:
 *   fermentiq/<id>/state          — JSON: all sensor values
 *   fermentiq/<id>/phase          — string: current fermentation phase
 *   fermentiq/<id>/abv            — float: estimated ABV
 *   fermentiq/<id>/spoilage_risk  — int: 0-100 risk score
 *   fermentiq/<id>/health         — float: 0-100 health score
 *
 * Subscribed topics:
 *   fermentiq/<id>/config/set     — JSON: configuration commands
 *   fermentiq/<id>/command        — string: start/stop/calibrate
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: GPL-3.0
 */

#include "wifi_mqtt.h"
#include "../board.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "mqtt_client.h"
#include "nvs_flash.h"
#include <string.h>
#include <stdio.h>

static const char *TAG = "wifi_mqtt";

static esp_mqtt_client_handle_t s_mqtt_client = NULL;
static bool s_wifi_connected = false;
static bool s_mqtt_connected = false;
static char s_device_id[16] = {0};

/* ------------------------------------------------------------------- */
/* Generate device ID from MAC address                                 */
/* ------------------------------------------------------------------- */
static void generate_device_id(void)
{
    uint8_t mac[6];
    esp_wifi_get_mac(WIFI_IF_STA, mac);
    snprintf(s_device_id, sizeof(s_device_id), "ftq-%02x%02x%02x",
             mac[3], mac[4], mac[5]);
}

/* ------------------------------------------------------------------- */
/* WiFi event handler                                                  */
/* ------------------------------------------------------------------- */
static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                                int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT) {
        switch (event_id) {
        case WIFI_EVENT_STA_START:
            esp_wifi_connect();
            ESP_LOGI(TAG, "WiFi STA started, connecting...");
            break;
        case WIFI_EVENT_STA_DISCONNECTED:
            s_wifi_connected = false;
            ESP_LOGW(TAG, "WiFi disconnected, retrying...");
            esp_wifi_connect();
            break;
        default:
            break;
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "WiFi connected! IP: " IPSTR, IP2STR(&event->ip_info.ip));
        s_wifi_connected = true;
        if (s_mqtt_client)
            esp_mqtt_client_start(s_mqtt_client);
    }
}

/* ------------------------------------------------------------------- */
/* MQTT event handler                                                  */
/* ------------------------------------------------------------------- */
static void mqtt_event_handler(void *arg, esp_event_base_t base,
                                int32_t id, void *data)
{
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)data;

    switch (id) {
    case MQTT_EVENT_CONNECTED:
        s_mqtt_connected = true;
        ESP_LOGI(TAG, "MQTT connected to broker");
        /* Subscribe to config/command topics */
        char topic[64];
        snprintf(topic, sizeof(topic), "fermentiq/%s/config/set", s_device_id);
        esp_mqtt_client_subscribe(s_mqtt_client, topic, 1);
        snprintf(topic, sizeof(topic), "fermentiq/%s/command", s_device_id);
        esp_mqtt_client_subscribe(s_mqtt_client, topic, 1);
        break;

    case MQTT_EVENT_DISCONNECTED:
        s_mqtt_connected = false;
        ESP_LOGW(TAG, "MQTT disconnected");
        break;

    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT rx: %.*s = %.*s",
                 (int)event->topic_len, event->topic,
                 (int)event->data_len, event->data);
        /* Process config/command — in full implementation, parse JSON
         * and update g_state accordingly */
        break;

    default:
        break;
    }
}

/* ------------------------------------------------------------------- */
/* Load WiFi credentials from NVS                                      */
/* ------------------------------------------------------------------- */
static int load_wifi_creds(char *ssid, size_t ssid_len,
                           char *pass, size_t pass_len,
                           char *broker, size_t broker_len)
{
    nvs_handle_t h;
    if (nvs_open("fermentiq", NVS_READONLY, &h) != ESP_OK)
        return -1;

    int rc = 0;
    if (nvs_get_str(h, "wifi_ssid", ssid, &ssid_len) != ESP_OK ||
        nvs_get_str(h, "wifi_pass", pass, &pass_len) != ESP_OK) {
        rc = -1;
    }
    size_t broker_sz = broker_len;
    if (nvs_get_str(h, "mqtt_broker", broker, &broker_sz) != ESP_OK) {
        strncpy(broker, "mqtt://homeassistant.local", broker_len);
    }

    nvs_close(h);
    return rc;
}

/* ------------------------------------------------------------------- */
/* Build JSON state payload                                            */
/* ------------------------------------------------------------------- */
static void build_state_json(const fermentiq_state_t *state, char *buf, size_t len)
{
    snprintf(buf, len,
        "{"
        "\"batch\":\"%s\","
        "\"type\":\"%s\","
        "\"active\":%s,"
        "\"age_hours\":%lu,"
        "\"impedance\":{"
            "\"cell_density\":%.2e,"
            "\"z_mag_1k\":%.1f,"
            "\"z_mag_10k\":%.1f,"
            "\"z_mag_100k\":%.1f,"
            "\"cole_alpha\":%.3f"
        "},"
        "\"co2\":{"
            "\"ppm\":%u,"
            "\"cer_mmol_lh\":%.3f,"
            "\"dissolved_g_l\":%.2f"
        "},"
        "\"ph\":{"
            "\"value\":%.2f,"
            "\"rate_ph_h\":%.4f"
        "},"
        "\"temperature\":{"
            "\"liquid_c\":%.2f,"
            "\"ambient_c\":%.1f,"
            "\"humidity_pct\":%.1f,"
            "\"dew_point_c\":%.1f"
        "},"
        "\"acoustic\":{"
            "\"bubble_rate\":%.1f,"
            "\"centroid_hz\":%.0f,"
            "\"rms\":%.4f"
        "},"
        "\"fusion\":{"
            "\"phase\":\"%s\","
            "\"abv_pct\":%.2f,"
            "\"attenuation_pct\":%.1f,"
            "\"spoilage_risk\":%d,"
            "\"health_score\":%.0f"
        "},"
        "\"battery\":{"
            "\"soc_pct\":%.0f,"
            "\"voltage_mv\":%.0f,"
            "\"usb_connected\":%s"
        "}"
        "}",
        state->batch_name,
        ferm_type_names[state->type],
        state->active ? "true" : "false",
        (unsigned long)state->fusion.batch_age_hours,
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
        state->acoustic.rms_level,
        phase_names[state->fusion.phase],
        state->fusion.abv_estimate,
        state->fusion.attenuation,
        state->fusion.spoilage_risk,
        state->fusion.health_score,
        state->battery_soc,
        state->battery_mv,
        state->usb_connected ? "true" : "false"
    );
}

/* ------------------------------------------------------------------- */
/* Publish Home Assistant discovery messages                           */
/* ------------------------------------------------------------------- */
static void publish_ha_discovery(void)
{
    if (!s_mqtt_connected)
        return;

    char topic[128];
    char payload[512];

    /* Define sensor entities for HA auto-discovery */
    const struct {
        const char *name;
        const char *unit;
        const char *value_template;
        const char *icon;
    } sensors[] = {
        {"Liquid Temperature", "°C",      "{{ value_json.temperature.liquid_c }}",      "mdi:thermometer"},
        {"CO2",                "ppm",     "{{ value_json.co2.ppm }}",                    "mdi:molecule-co2"},
        {"CO2 Evolution Rate", "mmol/L/h", "{{ value_json.co2.cer_mmol_lh }}",           "mdi:trending-up"},
        {"pH",                 "",        "{{ value_json.ph.value }}",                   "mdi:water"},
        {"Cell Density",       "cells/mL","{{ value_json.impedance.cell_density }}",     "mdi:bacteria"},
        {"Bubble Rate",        "/min",    "{{ value_json.acoustic.bubble_rate }}",       "mdi:chart-bubble"},
        {"ABV Estimate",       "%",       "{{ value_json.fusion.abv_pct }}",             "mdi:glass-wine"},
        {"Spoilage Risk",      "",        "{{ value_json.fusion.spoilage_risk }}",       "mdi:alert"},
        {"Health Score",       "",        "{{ value_json.fusion.health_score }}",        "mdi:heart-pulse"},
        {"Battery",            "%",       "{{ value_json.battery.soc_pct }}",            "mdi:battery"},
    };

    for (int i = 0; i < (int)(sizeof(sensors)/sizeof(sensors[0])); i++) {
        snprintf(topic, sizeof(topic),
            "homeassistant/sensor/fermentiq/%s/config", sensors[i].name);
        snprintf(payload, sizeof(payload),
            "{\"name\":\"FermenTiq %s\","
            "\"state_topic\":\"fermentiq/%s/state\","
            "\"unit_of_measurement\":\"%s\","
            "\"value_template\":\"%s\","
            "\"icon\":\"%s\","
            "\"unique_id\":\"ftq_%s_%d\"}",
            sensors[i].name, s_device_id, sensors[i].unit,
            sensors[i].value_template, sensors[i].icon,
            s_device_id, i);
        esp_mqtt_client_publish(s_mqtt_client, topic, payload, 0, 1, true);
    }
}

/* ------------------------------------------------------------------- */
/* Initialization                                                      */
/* ------------------------------------------------------------------- */
int wifi_mqtt_init(void)
{
    char ssid[33] = {0}, pass[65] = {0}, broker[129] = {0};

    if (load_wifi_creds(ssid, sizeof(ssid), pass, sizeof(pass),
                        broker, sizeof(broker)) != 0) {
        ESP_LOGW(TAG, "No WiFi credentials in NVS — BLE-only mode");
        return 0;  /* Not an error — device works in BLE-only mode */
    }

    /* Initialize WiFi */
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    esp_event_handler_instance_t any_id, got_ip;
    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                         &wifi_event_handler, NULL, &any_id);
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                         &wifi_event_handler, NULL, &got_ip);

    wifi_config_t wifi_cfg = {0};
    strncpy((char *)wifi_cfg.sta.ssid, ssid, sizeof(wifi_cfg.sta.ssid) - 1);
    strncpy((char *)wifi_cfg.sta.password, pass, sizeof(wifi_cfg.sta.password) - 1);

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_cfg);
    esp_wifi_start();

    generate_device_id();

    /* Initialize MQTT client */
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.uri = broker,
        .credentials.client_id = s_device_id,
    };
    s_mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    if (s_mqtt_client) {
        esp_mqtt_client_register_event(s_mqtt_client, ESP_EVENT_ANY_ID,
                                        mqtt_event_handler, NULL);
    }

    ESP_LOGI(TAG, "WiFi+MQTT initialized: SSID=%s, broker=%s, device=%s",
             ssid, broker, s_device_id);
    return 0;
}

/* ------------------------------------------------------------------- */
/* Publish state to MQTT                                               */
/* ------------------------------------------------------------------- */
int wifi_mqtt_publish(const fermentiq_state_t *state)
{
    if (!s_mqtt_connected || !s_mqtt_client)
        return -1;

    char json[1024];
    build_state_json(state, json, sizeof(json));

    char topic[64];
    snprintf(topic, sizeof(topic), "fermentiq/%s/state", s_device_id);
    esp_mqtt_client_publish(s_mqtt_client, topic, json, 0, 1, false);

    /* Publish individual topics for simple integrations */
    snprintf(topic, sizeof(topic), "fermentiq/%s/phase", s_device_id);
    esp_mqtt_client_publish(s_mqtt_client, topic,
                            phase_names[state->fusion.phase], 0, 1, false);

    snprintf(topic, sizeof(topic), "fermentiq/%s/abv", s_device_id);
    char abv_str[16];
    snprintf(abv_str, sizeof(abv_str), "%.2f", state->fusion.abv_estimate);
    esp_mqtt_client_publish(s_mqtt_client, topic, abv_str, 0, 1, false);

    snprintf(topic, sizeof(topic), "fermentiq/%s/spoilage_risk", s_device_id);
    char risk_str[8];
    snprintf(risk_str, sizeof(risk_str), "%d", state->fusion.spoilage_risk);
    esp_mqtt_client_publish(s_mqtt_client, topic, risk_str, 0, 1, false);

    return 0;
}

/* ------------------------------------------------------------------- */
/* Subscribe to a topic                                                */
/* ------------------------------------------------------------------- */
int wifi_mqtt_subscribe(const char *topic)
{
    if (!s_mqtt_connected || !s_mqtt_client || !topic)
        return -1;
    return esp_mqtt_client_subscribe(s_mqtt_client, topic, 1) < 0 ? -1 : 0;
}