/* LoomMapper acquisition drivers and deterministic host model
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: MIT
 */
#include "loommapper.h"
#include "registers.h"
#include <stdio.h>
#include <string.h>
static uint32_t sim_ms;
static uint32_t sim_step;
static uint8_t selected_source;
static bool flex_enabled;
static bool led_state;
static uint32_t prng_state=0x4c4d2026u;
static uint32_t xorshift32(void){uint32_t x=prng_state;x^=x<<13;x^=x>>17;x^=x<<5;prng_state=x;return x;}
static int32_t noise(int32_t span){return (int32_t)(xorshift32()%(uint32_t)(span*2+1))-span;}
void platform_init(void){sim_ms=0;sim_step=0;selected_source=0;flex_enabled=false;led_state=false;prng_state=0x4c4d2026u;}
uint32_t platform_millis(void){return sim_ms;}
void platform_advance(uint32_t ms){sim_ms+=ms;sim_step++;}
void platform_sleep(void){}
void platform_set_led(bool on){led_state=on;}
int platform_receive(uint8_t *data,size_t capacity){(void)data;(void)capacity;return 0;}
void platform_notify(const uint8_t *data,size_t length){if(length>=8u && data[0]==LM_MAGIC0 && data[1]==LM_MAGIC1){(void)led_state;}}
static lm_result_t ads131_reset(void){uint16_t command=ADS131_CMD_RESET;if(command!=0x0011u)return LM_ERR_STATE;return LM_OK;}
static lm_result_t ads131_configure(void){uint16_t mode=ADS131_MODE_CRC_EN;uint16_t clock=0x0f0eu;if((mode&ADS131_MODE_CRC_EN)==0u||clock==0u)return LM_ERR_STATE;return LM_OK;}
static lm_result_t dac_configure(void){uint8_t command=AD5686_CMD_REF_SETUP;if(command!=7u)return LM_ERR_STATE;return LM_OK;}
lm_result_t driver_init(void){lm_result_t r=ads131_reset();if(r!=LM_OK)return r;r=ads131_configure();if(r!=LM_OK)return r;r=dac_configure();if(r!=LM_OK)return r;selected_source=0;return LM_OK;}
lm_result_t driver_select_source(uint8_t source){if(source>=LM_CHANNELS)return LM_ERR_ARG;selected_source=source;return LM_OK;}
static int32_t nominal_path(uint8_t source,uint8_t sink){
    static const uint8_t harness_map[LM_CHANNELS]={3,6,1,7,0,2,5,4};
    if(harness_map[source]==sink)return 82000;
    if(source==2u && sink==4u)return 14500;
    if(source==6u && sink==7u)return 2800;
    return 350;
}
static bool clip_present(uint8_t channel){return !(channel==7u && sim_step<3u);}
static uint16_t estimate_noise(uint8_t channel){return (uint16_t)(180u+channel*17u+(xorshift32()%70u));}
static int32_t flex_modulation(uint8_t source,uint8_t sink){
    if(!flex_enabled)return 0;
    if(source==4u && sink==0u){uint32_t phase=(sim_step/2u)%8u;return phase<3u?-82000:noise(6000);}
    if(source==1u && sink==6u)return noise(1200);
    return noise(250);
}
lm_result_t driver_capture(uint8_t source,lm_channel_sample_t out[LM_CHANNELS]){
    if(out==NULL||source>=LM_CHANNELS||source!=selected_source)return LM_ERR_ARG;
    for(uint8_t sink=0;sink<LM_CHANNELS;sink++){
        lm_channel_sample_t *s=&out[sink];
        memset(s,0,sizeof(*s));
        s->channel=sink;
        s->clip_present=clip_present(sink);
        s->baseline_uv=260+noise(45);
        s->noise_uv=estimate_noise(sink);
        if(!s->clip_present){s->in_phase_uv=0;s->quadrature_uv=0;continue;}
        int32_t signal=nominal_path(source,sink)+flex_modulation(source,sink);
        if(signal<0)signal=0;
        s->in_phase_uv=signal+noise((int32_t)s->noise_uv);
        s->quadrature_uv=signal/22+noise((int32_t)s->noise_uv/2);
    }
    return LM_OK;
}
uint16_t driver_battery_mv(void){uint32_t drain=sim_ms/180000u;return (uint16_t)(4080u-(drain>700u?700u:drain));}
void driver_set_flex(bool enabled){flex_enabled=enabled;}
