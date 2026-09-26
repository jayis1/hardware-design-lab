/*
 * CasterEcho application state machine and self-tests.
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: MIT
 */
#include "casterecho.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static ce_capture_t capture;
static ce_history_t history;
static ce_calibration_t calibration;
static uint8_t radio_input[CE_PACKET_MAX];
static uint8_t radio_output[CE_PACKET_MAX];

static void defaults(void){
    memset(&calibration,0,sizeof(calibration));
    calibration.baseline_rms_g=0.05f;calibration.baseline_wheel_order=0.0001f;
    calibration.baseline_high_band=0.0001f;calibration.baseline_drag=0.08f;
    calibration.wheel_circumference_mm=CE_WHEEL_DIAMETER_MM*3.14159265359f;
    calibration.crc32=ce_crc32(&calibration,sizeof(calibration)-sizeof(calibration.crc32));
}

static ce_result_t finish_window(void){
    ce_pass_t pass;memset(&pass,0,sizeof(pass));
    pass.started_ms=capture.started_ms;pass.duration_ms=platform_millis()-capture.started_ms;
    pass.harvested_mj=platform_harvested_mj();pass.samples=(uint16_t)capture.count;
    ce_result_t result=analytics_compute(&capture,&calibration,&pass.features);
    if(result!=CE_OK)return result;
    if(calibration.accepted_passes<CE_BASELINE_MIN_PASSES)analytics_update_baseline(&calibration,&pass.features);
    result=history_append(&history,&pass);
    if(result==CE_OK){printf("pass=%u health=%u flags=0x%04x rms=%.4f speed=%.2f\n",pass.sequence,pass.features.health_score,pass.features.flags,pass.features.vibration_rms_g,pass.features.mean_speed_mps);}
    capture_init(&capture);return result;
}

static void service_radio(void){
    int amount=platform_ble_receive(radio_input,sizeof(radio_input));if(amount<=0)return;
    ce_packet_t request,response;
    if(protocol_decode(radio_input,(size_t)amount,&request)!=CE_OK)return;
    (void)protocol_handle(&request,&response,&history,&calibration);
    size_t encoded=protocol_encode(&response,radio_output,sizeof(radio_output));
    if(encoded>0u)platform_ble_notify(radio_output,encoded);
}

static int assert_true(bool condition,const char *name){if(!condition){fprintf(stderr,"FAIL: %s\n",name);return 1;}printf("ok: %s\n",name);return 0;}
static int run_self_tests(void){
    int failures=0;platform_init();defaults();history_init(&history);capture_init(&capture);
    failures+=assert_true(sensors_init()==CE_OK,"sensor identity and configuration");
    failures+=assert_true(fabsf(sensors_speed_from_period(100000u,400.0f)-4.0f)<0.001f,"hall period speed conversion");
    const char vector[]="123456789";failures+=assert_true(ce_crc32(vector,9u)==0xcbf43926u,"standard CRC-32 vector");
    ce_packet_t p={0},decoded={0};p.type=CE_CMD_INFO;p.version=CE_PROTOCOL_VERSION;p.sequence=42u;p.payload_length=3u;memcpy(p.payload,"abc",3u);
    size_t n=protocol_encode(&p,radio_output,sizeof(radio_output));
    failures+=assert_true(n==17u&&protocol_decode(radio_output,n,&decoded)==CE_OK&&decoded.sequence==42u,"protocol round trip");
    radio_output[8]^=0x40u;failures+=assert_true(protocol_decode(radio_output,n,&decoded)==CE_ERR_CRC,"protocol rejects corruption");
    capture_init(&capture);
    for(size_t i=0;i<CE_WINDOW_SAMPLES;++i){ce_sample_t s;if(sensors_read(&s)!=CE_OK){++failures;break;}capture_push(&capture,&s);platform_sim_advance();}
    ce_features_t f;failures+=assert_true(analytics_compute(&capture,&calibration,&f)==CE_OK,"feature extraction");
    failures+=assert_true(isfinite(f.vibration_rms_g)&&f.health_score<=100u,"bounded health result");
    ce_pass_t record={0};record.features=f;failures+=assert_true(history_append(&history,&record)==CE_OK,"history append");
    failures+=assert_true(history_latest(&history)!=NULL&&history_latest(&history)->sequence==1u,"history retrieval");
    ce_packet_t q={0},r={0};q.type=CE_CMD_LATEST;q.sequence=7u;
    failures+=assert_true(protocol_handle(&q,&r,&history,&calibration)==CE_OK&&r.type==CE_MSG_PASS,"latest command");
    printf("self-test: %s (%d failures)\n",failures==0?"PASS":"FAIL",failures);return failures==0?EXIT_SUCCESS:EXIT_FAILURE;
}

int main(int argc,char **argv){
    if(argc>1&&strcmp(argv[1],"--self-test")==0)return run_self_tests();
    platform_init();defaults();history_init(&history);capture_init(&capture);
    if(sensors_init()!=CE_OK){fprintf(stderr,"sensor initialization failed\n");return EXIT_FAILURE;}
    for(unsigned windows=0u;windows<3u;){
        ce_sample_t sample;
        if(sensors_read(&sample)==CE_OK&&capture_push(&capture,&sample)){if(finish_window()!=CE_OK)return EXIT_FAILURE;++windows;}
        service_radio();platform_sim_advance();
    }
    const ce_pass_t *latest=history_latest(&history);
    if(latest!=NULL)printf("CasterEcho ready: %u passes, latest health %u/100\n",history.count,latest->features.health_score);
    platform_set_led(false);platform_sleep();return EXIT_SUCCESS;
}
