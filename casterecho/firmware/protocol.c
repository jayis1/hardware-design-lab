/*
 * CasterEcho framed BLE protocol and persistent history.
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: MIT
 */
#include "casterecho.h"
#include <string.h>

static void put_u16(uint8_t *p,uint16_t v){p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);}
static void put_u32(uint8_t *p,uint32_t v){p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);p[2]=(uint8_t)(v>>16);p[3]=(uint8_t)(v>>24);}
static uint16_t get_u16(const uint8_t*p){return (uint16_t)p[0]|((uint16_t)p[1]<<8);}
static uint32_t get_u32(const uint8_t*p){return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}

uint32_t ce_crc32(const void *data,size_t length){
    const uint8_t *bytes=(const uint8_t*)data; uint32_t crc=0xffffffffu;
    for(size_t i=0;i<length;++i){crc^=bytes[i];for(unsigned b=0;b<8u;++b)crc=(crc>>1)^((crc&1u)?0xedb88320u:0u);}return ~crc;
}

void history_init(ce_history_t *h){if(h==NULL)return;memset(h,0,sizeof(*h));h->next_sequence=1u;}
ce_result_t history_append(ce_history_t*h,ce_pass_t*p){
    if(h==NULL||p==NULL)return CE_ERR_ARGUMENT;
    p->sequence=h->next_sequence++; p->crc32=ce_crc32(p,sizeof(*p)-sizeof(p->crc32));
    h->records[h->head]=*p;
    ce_result_t result=platform_store(h->head,p,sizeof(*p));
    h->head=(uint16_t)((h->head+1u)%CE_HISTORY_RECORDS); if(h->count<CE_HISTORY_RECORDS)++h->count;
    return result==CE_ERR_STORAGE?CE_OK:result;
}
const ce_pass_t *history_latest(const ce_history_t*h){if(h==NULL||h->count==0u)return NULL;return &h->records[(h->head+CE_HISTORY_RECORDS-1u)%CE_HISTORY_RECORDS];}
const ce_pass_t *history_at(const ce_history_t*h,uint16_t age){if(h==NULL||age>=h->count)return NULL;return &h->records[(h->head+CE_HISTORY_RECORDS-1u-age)%CE_HISTORY_RECORDS];}

size_t protocol_encode(const ce_packet_t*p,uint8_t*out,size_t cap){
    if(p==NULL||out==NULL||p->payload_length>sizeof(p->payload))return 0u;
    size_t total=14u+p->payload_length; if(cap<total)return 0u;
    put_u16(out,CE_MAGIC);out[2]=p->version;out[3]=p->type;put_u16(&out[4],p->payload_length);put_u32(&out[6],p->sequence);
    memcpy(&out[10],p->payload,p->payload_length);put_u32(&out[10+p->payload_length],ce_crc32(out,10u+p->payload_length));return total;
}
ce_result_t protocol_decode(const uint8_t*in,size_t n,ce_packet_t*p){
    if(in==NULL||p==NULL||n<14u)return CE_ERR_ARGUMENT;
    uint16_t len=get_u16(&in[4]);if(get_u16(in)!=CE_MAGIC||len>sizeof(p->payload)||n!=14u+len)return CE_ERR_RANGE;
    if(get_u32(&in[10+len])!=ce_crc32(in,10u+len))return CE_ERR_CRC;
    memset(p,0,sizeof(*p));p->version=in[2];p->type=in[3];p->payload_length=len;p->sequence=get_u32(&in[6]);memcpy(p->payload,&in[10],len);return CE_OK;
}

static void encode_pass(const ce_pass_t*p,ce_packet_t*r){
    const size_t amount=sizeof(*p)<sizeof(r->payload)?sizeof(*p):sizeof(r->payload);
    memcpy(r->payload,p,amount);r->payload_length=(uint16_t)amount;
}
ce_result_t protocol_handle(const ce_packet_t*q,ce_packet_t*r,ce_history_t*h,ce_calibration_t*c){
    if(q==NULL||r==NULL||h==NULL||c==NULL)return CE_ERR_ARGUMENT;
    memset(r,0,sizeof(*r));r->version=CE_PROTOCOL_VERSION;r->sequence=q->sequence;
    switch(q->type){
    case CE_CMD_INFO:
        r->type=CE_MSG_INFO;r->payload_length=(uint16_t)strlen(CE_FW_VERSION)+1u;memcpy(r->payload,CE_FW_VERSION,r->payload_length);return CE_OK;
    case CE_CMD_LATEST:{const ce_pass_t*p=history_latest(h);if(p==NULL)break;r->type=CE_MSG_PASS;encode_pass(p,r);return CE_OK;}
    case CE_CMD_HISTORY:{uint16_t age=q->payload_length>=2u?get_u16(q->payload):0u;const ce_pass_t*p=history_at(h,age);if(p==NULL)break;r->type=CE_MSG_HISTORY;encode_pass(p,r);return CE_OK;}
    case CE_CMD_CALIBRATE:{const ce_pass_t*p=history_latest(h);if(p==NULL)break;analytics_update_baseline(c,&p->features);r->type=CE_MSG_ACK;r->payload[0]=1u;r->payload_length=1u;return CE_OK;}
    case CE_CMD_CLEAR: history_init(h);r->type=CE_MSG_ACK;r->payload[0]=1u;r->payload_length=1u;return CE_OK;
    case CE_CMD_SET_WHEEL: if(q->payload_length==sizeof(float)){float mm;memcpy(&mm,q->payload,sizeof(mm));if(mm>=150.0f&&mm<=1000.0f){c->wheel_circumference_mm=mm;r->type=CE_MSG_ACK;r->payload_length=0u;return CE_OK;}}break;
    default: break;
    }
    r->type=CE_MSG_ERROR;r->payload[0]=(uint8_t)CE_ERR_STATE;r->payload_length=1u;return CE_ERR_STATE;
}
void protocol_make_live(const ce_sample_t*s,ce_packet_t*p){memset(p,0,sizeof(*p));p->type=CE_MSG_LIVE;p->version=CE_PROTOCOL_VERSION;p->sequence=s->timestamp_ms;memcpy(p->payload,s,sizeof(*s));p->payload_length=(uint16_t)sizeof(*s);}
