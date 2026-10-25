/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include "wire.h"
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
static uint32_t now;
static void hold(Engine *e, uint8_t mask, unsigned ms)
{
    for (unsigned i=0; i<ms; ++i) engine_sample(e, ++now, mask);
}
static void baseline(Engine *e)
{
    now=0;
    engine_init(e,now);
    hold(e,P_REST,30);
    assert(e->phase==READY);
}
static void armed(Engine *e)
{
    baseline(e);
    hold(e,KEY_A|P_DOWN,30);
    assert(e->phase==CAPTURE);
}
int main(void)
{
    Engine e;
    armed(&e);
    hold(&e,KEY_B|P_DOWN,20); /* A release and B onset simultaneous. */
    hold(&e,KEY_B|P_REST,20);
    hold(&e,KEY_B|P_DOWN,20);
    assert(e.completed==1 && e.last.outcome==RESULT_COMPLETE);
    assert(e.last.key_gap==0 && e.last.release_delta==20);
    assert(e.last.down_delta==40 && (e.last.flags&FLAG_SIMULTANEOUS));
    hold(&e,KEY_A|P_DOWN,30);
    assert(e.phase==WAIT_BASELINE); /* held levels cannot start a new trial */
    armed(&e);
    hold(&e,P_REST,40); /* release pedal before the new key */
    hold(&e,KEY_B|P_REST,20);
    hold(&e,KEY_B|P_DOWN,20);
    assert(e.last.key_gap==40 && e.last.release_delta==-40);
    armed(&e);
    hold(&e,KEY_A|KEY_B|P_DOWN,20);
    hold(&e,KEY_B|P_REST,20);
    hold(&e,KEY_B|P_DOWN,20);
    assert(e.last.key_gap==-20);
    armed(&e);
    hold(&e,P_DOWN|P_REST,20);
    assert(e.rejected==1 && (e.last.flags&FLAG_CONTRADICTION));
    armed(&e);
    hold(&e,KEY_A|P_DOWN,TRIAL_MS+20);
    assert(e.rejected==1 && (e.last.flags&FLAG_TIMEOUT));
    armed(&e);
    now+=20;
    engine_sample(&e,now,0);
    assert(e.gaps==1 && e.phase==WAIT_BASELINE && e.rejected==1);
    assert(e.levels==(KEY_A|P_DOWN)); /* gap doesn't qualify a glitch */
    armed(&e);
    hold(&e,KEY_B|P_DOWN,20);
    hold(&e,P_REST,20);
    assert(e.rejected==1 && (e.last.flags&FLAG_KEY_RELEASE));
    armed(&e);
    hold(&e,P_DOWN,20);
    hold(&e,KEY_A|P_DOWN,20);
    assert(e.rejected==1 && (e.last.flags&FLAG_REATTACK));
    baseline(&e);
    hold(&e,KEY_A|P_DOWN,3);
    hold(&e,P_REST,20);
    assert(e.phase==READY && e.attempts==0);
    now=UINT32_MAX-10;
    engine_init(&e,now);
    hold(&e,P_REST,30);
    assert(e.phase==READY && e.gaps==0);
    e.gaps=UINT32_MAX;
    now+=20; engine_sample(&e,now,0);
    assert(e.gaps==UINT32_MAX);
    armed(&e);
    hold(&e,KEY_B|P_DOWN,20);
    hold(&e,P_REST,20);
    assert(e.phase==READY); /* aborted trial already at baseline */
    hold(&e,KEY_A|P_DOWN,20);
    assert(e.phase==CAPTURE);
    hold(&e,P_REST,TRIAL_MS+30);
    assert(e.phase==READY); /* timeout at baseline must recover */
    armed(&e);
    hold(&e,P_DOWN,7); /* A opens, followed by unrelated pedal movement */
    hold(&e,0,23);
    hold(&e,KEY_B,20);
    hold(&e,KEY_B|P_REST,20);
    hold(&e,KEY_B|P_DOWN,20);
    assert(e.last.key_gap==30); /* each input has its own qualification */
    char frame[256];
    assert(wire_crc("123456789",9)==0x29b1);
    size_t len=wire_status(&e,1,frame,sizeof frame);
    assert(len>0 && frame[len-1]=='\n');
    assert(wire_status(&e,1,frame,8)==0);
    assert(wire_status(NULL,1,frame,sizeof frame)==0);
    puts("engine: ordered, early, overlap, faults, debounce, wrap and CRC passed");
}
