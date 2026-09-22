/* LoomMapper versioned binary protocol
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: MIT
 */
#include "loommapper.h"
#include <string.h>
enum { MSG_STATUS=0x01,MSG_EDGES=0x02,CMD_START=0x80,CMD_STOP=0x81,CMD_FLEX=0x82,CMD_CLEAR=0x83 };
static void put16(uint8_t *p,uint16_t v){p[0]=(uint8_t)(v&0xffu);p[1]=(uint8_t)(v>>8);}
static void put32(uint8_t *p,uint32_t v){p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);p[2]=(uint8_t)(v>>16);p[3]=(uint8_t)(v>>24);}
static uint16_t get16(const uint8_t *p){return (uint16_t)p[0]|((uint16_t)p[1]<<8);}
static bool frame_header(uint8_t type,uint16_t payload,uint8_t *out,size_t capacity){if(out==NULL||capacity<(size_t)payload+9u)return false;out[0]=LM_MAGIC0;out[1]=LM_MAGIC1;out[2]=LM_PROTOCOL_VERSION;out[3]=type;put16(out+4,payload);return true;}
static size_t finish(uint8_t *out,size_t payload){size_t body=6u+payload;uint16_t crc=lm_crc16(out,body);put16(out+body,crc);out[body+2u]='\n';return body+3u;}
size_t protocol_encode_status(const lm_context_t *ctx,const lm_graph_t *graph,uint8_t *out,size_t capacity){if(ctx==NULL||graph==NULL||!frame_header(MSG_STATUS,18u,out,capacity))return 0;out[6]=(uint8_t)ctx->state;out[7]=ctx->active_source;put16(out+8,ctx->battery_mv);put16(out+10,ctx->fault_count);put16(out+12,graph->edge_count);put32(out+14,graph->scan_sequence);put32(out+18,graph->graph_crc);out[22]=graph->overflow?1u:0u;out[23]=0u;return finish(out,18u);}
size_t protocol_encode_edges(const lm_graph_t *graph,uint8_t start,uint8_t *out,size_t capacity){
    if(graph==NULL||out==NULL||start>=graph->edge_count)return 0;
    size_t max_records=(capacity>10u)?(capacity-10u)/10u:0u;
    size_t available=graph->edge_count-start;
    size_t count=available<max_records?available:max_records;
    uint16_t payload=(uint16_t)(1u+count*10u);
    if(!frame_header(MSG_EDGES,payload,out,capacity))return 0;
    out[6]=(uint8_t)count;
    size_t pos=7u;
    for(size_t i=0;i<count;i++){
        const lm_edge_t *e=&graph->edges[start+i];
        out[pos++]=e->source;out[pos++]=e->sink;out[pos++]=(uint8_t)e->kind;
        put16(out+pos,e->confidence_permille);pos+=2;
        put16(out+pos,e->variation_permille);pos+=2;
        int32_t mv=e->magnitude_uv/1000;
        put16(out+pos,(uint16_t)(int16_t)mv);pos+=2;
        out[pos++]=0u;
    }
    return finish(out,payload);
}
static lm_result_t validate(const uint8_t *packet,size_t length,uint8_t *type,uint16_t *payload){
    if(packet==NULL||length<9u||type==NULL||payload==NULL)return LM_ERR_ARG;
    if(packet[0]!=LM_MAGIC0||packet[1]!=LM_MAGIC1||packet[2]!=LM_PROTOCOL_VERSION)return LM_ERR_STATE;
    *type=packet[3];*payload=get16(packet+4);
    if(length!=(size_t)(*payload)+9u||packet[length-1u]!='\n')return LM_ERR_ARG;
    uint16_t expected=get16(packet+6u+*payload);
    if(lm_crc16(packet,6u+*payload)!=expected)return LM_ERR_CRC;
    return LM_OK;
}
lm_result_t protocol_handle(const uint8_t *packet,size_t length,lm_context_t *ctx,lm_graph_t *graph,lm_event_log_t *events){
    if(ctx==NULL||graph==NULL||events==NULL)return LM_ERR_ARG;
    uint8_t type=0;uint16_t payload=0;lm_result_t r=validate(packet,length,&type,&payload);if(r!=LM_OK)return r;
    switch(type){
        case CMD_START: if(payload!=0u)return LM_ERR_ARG;ctx->state=LM_BASELINE;ctx->active_source=0;ctx->settled_scans=0;driver_set_flex(false);return LM_OK;
        case CMD_STOP: if(payload!=0u)return LM_ERR_ARG;ctx->state=LM_IDLE;driver_set_flex(false);return LM_OK;
        case CMD_FLEX: if(payload!=1u)return LM_ERR_ARG;ctx->state=packet[6]?LM_FLEX_WATCH:LM_SCAN;driver_set_flex(packet[6]!=0u);return LM_OK;
        case CMD_CLEAR: if(payload!=0u)return LM_ERR_ARG;graph_init(graph);event_log_init(events);return LM_OK;
        default:return LM_ERR_STATE;
    }
}
