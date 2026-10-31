/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include "wire.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint32_t t;
static void hold(Engine *e, unsigned raw, unsigned count)
{
    while (count--) {
        engine_sample(e, ++t, raw);
        assert(engine_valid(e));
    }
}
static Engine ready(unsigned mask)
{
    Engine e;
    engine_init(&e);
    t=0;
    hold(&e,mask,80);
    return e;
}
static void transfers(void)
{
    /* Every independent direction and all four outcome combinations. */
    for (unsigned a=1;a<=2;a++) for (unsigned b=1;b<=2;b++) {
        for (unsigned x=1;x<=2;x++) for (unsigned y=1;y<=2;y++) {
            Engine e=ready(a|(b<<2));
            assert(e.holder[0].state==DOCKED && e.holder[1].state==DOCKED);
            hold(&e,0,40);
            assert(e.holder[0].state==TRANSIT && e.holder[1].state==TRANSIT);
            hold(&e,x|(y<<2),10);
            Holder *p=&e.holder[0], *q=&e.holder[1];
            assert(p->started==1 && q->started==1);
            assert(p->returned==(a==x) && p->delivered==(a!=x));
            assert(q->returned==(b==y) && q->delivered==(b!=y));
            assert(p->last_ms==40 && q->last_ms==40);
            assert(p->last_kind==(a==x?RETURNED:DELIVERED));
            assert(p->state==WAIT && q->state==WAIT);
        }
    }
}
static void boundaries(void)
{
    Engine e=ready(5);
    hold(&e,4,10);
    uint32_t start=e.holder[0].departure;
    while (t-start<DEADLINE_MS-10) hold(&e,4,1);
    hold(&e,6,10); /* Qualified arrival exactly at deadline loses to timeout. */
    assert(e.holder[0].abandoned==1 && e.holder[0].delivered==0);
    assert(e.holder[0].timeouts==1);
    e=ready(5); hold(&e,4,10); start=e.holder[0].departure;
    while (t-start<DEADLINE_MS-11) hold(&e,4,1);
    hold(&e,6,10);
    assert(e.holder[0].delivered==1 && e.holder[0].last_ms==DEADLINE_MS-1);
    /* Direct source/destination handover has no observed absence. */
    e=ready(5); hold(&e,6,10);
    assert(e.holder[0].started==0 && e.holder[0].ambiguities==1);
    /* Raw competing dock assertion blocks an otherwise qualified arrival. */
    e=ready(5); hold(&e,4,20); hold(&e,6,9); hold(&e,7,1);
    assert(e.holder[0].abandoned==1 && e.holder[0].delivered==0);
    /* Unrelated input chatter cannot delay holder A. */
    e=ready(5); hold(&e,4,10);
    for (unsigned i=0;i<20;i++) hold(&e,2|((i&1)?4:0),1);
    assert(e.holder[0].delivered==1);
    /* Pending raw release cannot earn baseline time. */
    e=ready(5); hold(&e,4,1); assert(e.holder[0].state==DOCKED);
    hold(&e,5,20); assert(e.holder[0].started==0);
}
static void failures(void)
{
    Engine e=ready(5); hold(&e,0,20);
    engine_sample(&e,t+=2,0);
    assert(engine_valid(&e));
    assert(e.gaps==1 && e.holder[0].abandoned==1 && e.holder[1].abandoned==1);
    assert(e.holder[0].state==WAIT && e.known==0);
    hold(&e,0,100); assert(e.holder[0].state==WAIT);
    hold(&e,5,80); assert(e.holder[0].state==DOCKED);
    hold(&e,7,10); assert(e.holder[0].ambiguities==1);
    hold(&e,7,100); assert(e.holder[0].ambiguities==1);
    e=ready(0); assert(e.holder[0].state==WAIT);
    hold(&e,3,100); assert(e.holder[0].state==WAIT);
    e=ready(5); hold(&e,0,20);
    engine_fault(&e,F_WATCHDOG);
    assert(engine_valid(&e) && e.halted && e.holder[0].abandoned==1);
    Engine old=e; hold(&e,15,100);
    assert(!memcmp(&old.holder,&e.holder,sizeof e.holder));
    e=ready(5); e.now=UINT32_MAX-20; t=e.now;
    hold(&e,0,30); hold(&e,10,10);
    assert(e.holder[0].delivered==1 && e.holder[1].delivered==1);
    /* Transactional rollback: B failure must undo A's speculative outcome. */
    e=ready(5); hold(&e,0,20);
    e.holder[1].sum_ms=UINT32_MAX-10;
    e.holder[1].delivered=1000000;
    e.holder[1].started=1000001;
    e.holder[1].max_ms=4999;
    e.holder[1].last_ms=4999;
    e.holder[1].last_kind=DELIVERED;
    assert(engine_valid(&e));
    hold(&e,10,10);
    assert(e.halted && (e.fault&F_EXHAUST));
    assert(e.holder[0].delivered==0 && e.holder[0].abandoned==1);
    assert(e.holder[1].delivered==1000000 && e.holder[1].abandoned==1);
}
static void encoder(void)
{
    Engine e=ready(5); char frame[768];
    assert(wire_crc("123456789",9)==0x29b1);
    assert(!wire_encode(&e,frame,8));
    size_t n=wire_encode(&e,frame,sizeof frame);
    assert(n && frame[n-1]=='\n' && !memcmp(frame,"CT1,",4));
    e.holder[0].started++;
    assert(!wire_encode(&e,frame,sizeof frame));
}
static void exhaustive_words(void)
{
    /* Independent dock/away oracle over all six-step words. Settling at
     * each endpoint removes qualification timing from this ordering test. */
    for (unsigned word=0; word<4096; ++word) {
        Engine e=ready(5);
        int location=1, origin=1;
        unsigned starts=0, deliveries=0, returns=0, lost=0, ambiguous=0;
        unsigned digits=word;
        for (unsigned step=0; step<6; ++step) {
            unsigned pair=digits&3; digits>>=2;
            if (location<0) {
                if (pair==1 || pair==2) location=(int)pair;
            } else if (location>0) {
                if (pair==0) { origin=location; location=0; starts++; }
                else if ((int)pair!=location) {
                    ambiguous++; location=pair==3?-1:(int)pair;
                }
            } else if (pair) {
                if (pair==3) { lost++; ambiguous++; location=-1; }
                else {
                    if ((int)pair==origin) returns++; else deliveries++;
                    location=(int)pair;
                }
            }
            hold(&e,pair|4,80);
            const Holder *h=&e.holder[0];
            assert(h->started==starts && h->delivered==deliveries && h->returned==returns);
            assert(h->abandoned==lost && h->ambiguities==ambiguous);
            assert(h->state==(location<0?WAIT:(location==0?TRANSIT:DOCKED)));
        }
    }
}
int main(int argc, char **argv)
{
    if (argc>1 && !strcmp(argv[1],"fixture")) {
        Engine e=ready(5); char frame[768];
        for (unsigned i=0;i<5;i++) {
            hold(&e,i==1?0:(i>=2?10:5),100);
            size_t n=wire_encode(&e,frame,sizeof frame);
            fwrite(frame,1,n,stdout);
        }
        return 0;
    }
    transfers(); boundaries(); failures(); encoder(); exhaustive_words();
    puts("PASS directions, return/delivery, independent holders, deadlines, raw guard, gaps, wrap, exhaustion, CRC");
}
