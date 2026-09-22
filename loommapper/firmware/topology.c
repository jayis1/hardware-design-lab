/* LoomMapper graph inference, confidence and event history
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: MIT
 */
#include "loommapper.h"
#include <stdlib.h>
#include <string.h>
static uint16_t clamp_permille(int32_t value){if(value<0)return 0;if(value>1000)return 1000;return (uint16_t)value;}
static int64_t magnitude_i32(int64_t value){return value<0?-value:value;}
uint16_t lm_crc16(const uint8_t *data,size_t length){uint16_t crc=0xffffu;for(size_t i=0;i<length;i++){crc^=(uint16_t)data[i]<<8;for(unsigned b=0;b<8;b++)crc=(crc&0x8000u)?(uint16_t)((crc<<1)^0x1021u):(uint16_t)(crc<<1);}return crc;}
uint32_t lm_crc32(const uint8_t *data,size_t length){uint32_t crc=0xffffffffu;for(size_t i=0;i<length;i++){crc^=data[i];for(unsigned b=0;b<8;b++)crc=(crc&1u)?(crc>>1)^0xedb88320u:crc>>1;}return ~crc;}
void graph_init(lm_graph_t *graph){if(graph!=NULL)memset(graph,0,sizeof(*graph));}
void event_log_init(lm_event_log_t *log){if(log!=NULL)memset(log,0,sizeof(*log));}
void event_log_append(lm_event_log_t *log,const lm_event_t *event){if(log==NULL||event==NULL)return;log->entries[log->head]=*event;log->head=(uint16_t)((log->head+1u)%LM_EVENT_CAPACITY);if(log->count<LM_EVENT_CAPACITY)log->count++;}
bool event_log_get(const lm_event_log_t *log,uint16_t newest_index,lm_event_t *event){if(log==NULL||event==NULL||newest_index>=log->count)return false;uint16_t pos=(uint16_t)((log->head+LM_EVENT_CAPACITY-1u-newest_index)%LM_EVENT_CAPACITY);*event=log->entries[pos];return true;}
lm_edge_kind_t classify_edge(const lm_channel_sample_t *sample,bool self_channel){
    if(sample==NULL||!sample->clip_present)return LM_EDGE_UNKNOWN;
    int64_t amplitude=magnitude_i32(sample->in_phase_uv-(int64_t)sample->baseline_uv);
    int64_t quadrature=magnitude_i32(sample->quadrature_uv);
    if(self_channel&&amplitude>LM_SHORT_THRESHOLD_UV)return LM_EDGE_SHORT;
    if(amplitude<LM_OPEN_THRESHOLD_UV)return LM_EDGE_OPEN;
    if(amplitude>LM_SHORT_THRESHOLD_UV)return LM_EDGE_SHORT;
    if(quadrature>amplitude/3)return LM_EDGE_CROSSTALK;
    return LM_EDGE_GOOD;
}
uint16_t edge_confidence(const lm_channel_sample_t *sample,lm_edge_kind_t kind){
    if(sample==NULL||!sample->clip_present||kind==LM_EDGE_UNKNOWN)return 0;
    int64_t amplitude=magnitude_i32(sample->in_phase_uv-(int64_t)sample->baseline_uv);
    int64_t snr=(amplitude*100)/(sample->noise_uv==0u?1:(int64_t)sample->noise_uv);
    int32_t base=snr>500?940:(int32_t)(440+snr);
    if(kind==LM_EDGE_OPEN)base=amplitude<LM_OPEN_THRESHOLD_UV/2?930:720;
    if(kind==LM_EDGE_CROSSTALK)base-=160;
    return clamp_permille(base);
}
const lm_edge_t *graph_find(const lm_graph_t *graph,uint8_t source,uint8_t sink){if(graph==NULL)return NULL;for(uint16_t i=0;i<graph->edge_count;i++)if(graph->edges[i].source==source&&graph->edges[i].sink==sink)return &graph->edges[i];return NULL;}
static lm_edge_t *graph_find_mutable(lm_graph_t *graph,uint8_t source,uint8_t sink){for(uint16_t i=0;i<graph->edge_count;i++)if(graph->edges[i].source==source&&graph->edges[i].sink==sink)return &graph->edges[i];return NULL;}
static lm_edge_t *graph_allocate(lm_graph_t *graph,uint8_t source,uint8_t sink,uint32_t now){
    if(graph->edge_count>=LM_MAX_EDGES){graph->overflow=true;return NULL;}
    lm_edge_t *edge=&graph->edges[graph->edge_count++];memset(edge,0,sizeof(*edge));edge->source=source;edge->sink=sink;edge->first_seen_ms=now;edge->last_seen_ms=now;edge->kind=LM_EDGE_UNKNOWN;return edge;
}
static uint16_t variation(int32_t previous,int32_t current){int64_t delta=magnitude_i32(current-(int64_t)previous);int64_t scale=magnitude_i32(previous);if(scale<1000)scale=1000;int64_t ratio=(delta*1000)/scale;return clamp_permille(ratio>INT32_MAX?INT32_MAX:(int32_t)ratio);}
static uint16_t severity_for(lm_edge_kind_t before,lm_edge_kind_t after,uint16_t var){if(after==LM_EDGE_SHORT)return 1000;if(after==LM_EDGE_INTERMITTENT)return (uint16_t)(600u+var/3u);if(after==LM_EDGE_OPEN&&before==LM_EDGE_GOOD)return 900;if(after!=before)return 500;return 0;}
static void update_one(lm_graph_t *graph,uint8_t source,const lm_channel_sample_t *sample,uint32_t now,lm_event_log_t *events){
    bool self=source==sample->channel;
    lm_edge_kind_t observed=classify_edge(sample,self);
    lm_edge_t *edge=graph_find_mutable(graph,source,sample->channel);
    if(edge==NULL)edge=graph_allocate(graph,source,sample->channel,now);
    if(edge==NULL)return;
    lm_edge_kind_t before=edge->kind;
    uint16_t var=variation(edge->magnitude_uv,sample->in_phase_uv);
    if(before==LM_EDGE_GOOD&&observed==LM_EDGE_OPEN&&magnitude_i32(edge->magnitude_uv-(int64_t)sample->in_phase_uv)>LM_INTERMITTENT_DELTA_UV)observed=LM_EDGE_INTERMITTENT;
    if(before==LM_EDGE_INTERMITTENT&&
       (observed==LM_EDGE_GOOD||observed==LM_EDGE_OPEN)&&
       (var>180u||edge->variation_permille>180u))observed=LM_EDGE_INTERMITTENT;
    edge->kind=observed;
    edge->confidence_permille=edge_confidence(sample,observed);
    edge->variation_permille=(uint16_t)((edge->variation_permille*3u+var)/4u);
    edge->magnitude_uv=(int32_t)(((int64_t)edge->magnitude_uv*3+sample->in_phase_uv)/4);
    edge->last_seen_ms=now;
    if(before!=LM_EDGE_UNKNOWN&&before!=edge->kind){lm_event_t event={now,source,sample->channel,before,edge->kind,severity_for(before,edge->kind,var)};event_log_append(events,&event);}
}
static void canonical_crc(lm_graph_t *graph){uint8_t bytes[LM_MAX_EDGES*6u];size_t used=0;for(uint16_t i=0;i<graph->edge_count&&used+6u<=sizeof(bytes);i++){const lm_edge_t *e=&graph->edges[i];bytes[used++]=e->source;bytes[used++]=e->sink;bytes[used++]=(uint8_t)e->kind;bytes[used++]=(uint8_t)(e->confidence_permille>>8);bytes[used++]=(uint8_t)e->confidence_permille;bytes[used++]=(uint8_t)(e->variation_permille>>2);}graph->graph_crc=lm_crc32(bytes,used);}
lm_result_t graph_update(lm_graph_t *graph,uint8_t source,const lm_channel_sample_t samples[LM_CHANNELS],uint32_t now,lm_event_log_t *events){if(graph==NULL||samples==NULL||events==NULL||source>=LM_CHANNELS)return LM_ERR_ARG;for(uint8_t sink=0;sink<LM_CHANNELS;sink++)update_one(graph,source,&samples[sink],now,events);graph->scan_sequence++;canonical_crc(graph);return LM_OK;}
