/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include "wire.h"
#include <assert.h>
#include <stdio.h>
static void hold(Engine *e, unsigned mask, unsigned n) {
    while(n--) { engine_sample(e,e->now+1,mask); assert(engine_valid(e)); }
}
static Engine ready(void) {
    Engine e; engine_init(&e); hold(&e,0,10); assert(e.phase==READY); return e;
}
int main(void) {
    Engine e=ready(); hold(&e,8,5); assert(e.phase==ACTIVE);
    hold(&e,9,5); hold(&e,8,5); hold(&e,12,5); hold(&e,8,5);
    hold(&e,9,5); hold(&e,8,5); hold(&e,0,5);
    assert(e.completed==1 && e.last_count==3);
    assert(e.last_word[0]==1 && e.last_word[1]==4 && e.last_word[2]==1);
    /* All words through four groups, against an independent array oracle. */
    for(unsigned code=0;code<2401;code++) {
        e=ready(); hold(&e,8,5); unsigned x=code;
        for(unsigned i=0;i<4;i++) { unsigned m=x%7+1; x/=7;
            hold(&e,8|m,5); hold(&e,8,5); assert(e.word[i]==m); }
        hold(&e,0,5); assert(e.completed==1 && e.last_count==4);
    }
    e=ready(); hold(&e,9,5); assert(e.starts==0); /* tied dock/open disallowed */
    e=ready(); hold(&e,8,5); hold(&e,9,5); hold(&e,1,5);
    assert(e.aborted==1 && e.completed==0);
    e=ready(); hold(&e,8,5); engine_sample(&e,e.now+2,8);
    assert(e.phase==HALT && e.aborted==1 && e.fault==F_GAP);
    e=ready(); hold(&e,8,5);
    for(unsigned i=0;i<17;i++){hold(&e,9,5);hold(&e,8,5);}
    assert(e.aborted==1 && e.last_count==0);
    e=ready(); hold(&e,8,5); e.age=DEADLINE-5; hold(&e,0,5);
    assert(e.aborted==1 && !e.completed);
    e=ready(); hold(&e,8,5); e.age=DEADLINE-6; hold(&e,0,5);
    assert(e.completed==1 && e.last_age==DEADLINE-1);
    e=ready(); hold(&e,8,5); hold(&e,1,5);
    assert(e.aborted==1 && e.count==0); /* opening tied to removal */
    e=ready(); e.now=0xffffffffu; hold(&e,0,1);
    assert(e.now==0 && e.phase==READY);
    e=ready(); hold(&e,8,5); hold(&e,11,5);
    assert(e.count==1 && e.word[0]==3); /* ties stay unordered */
    hold(&e,11,50); assert(e.count==1); /* held lid is not reuse */
    e=ready(); hold(&e,8,5); hold(&e,9,4); hold(&e,8,5);
    hold(&e,0,5); assert(e.completed==1 && e.last_count==0);
    e=ready(); e.starts=LIMIT; e.completed=LIMIT; hold(&e,8,5);
    assert(e.phase==HALT && e.fault==F_OVERFLOW);
    char frame[768]; assert(wire_encode(&e,frame,sizeof frame)>0);
    assert(wire_encode(&e,frame,5)==0);
    assert(wire_crc("123456789",9)==0x29b1);
    puts("PASS: exhaustive grouped words, boundaries, faults and CRC");
}
