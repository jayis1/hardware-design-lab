/* Author: jayis1. SPDX-License-Identifier: MIT */
#include <assert.h>
#include <stdio.h>
#include "engine.h"
#include "wire.h"
static uint32_t now;
static void hold(Engine *e, unsigned mask, unsigned n)
{
    while (n--) { engine_sample(e, ++now, mask); assert(engine_valid(e)); }
}
static void begin(Engine *e, unsigned mask)
{
    now=0; engine_init(e); hold(e, mask, 10);
}
int main(void)
{
    Engine e;
    const unsigned refs[]={0,1,3,7};
    /* Independent stable-word oracle: every four-word trace, including
     * malformed reference words and repeated DUT polarity. */
    for (unsigned word=0;word<65536;word++) {
        unsigned previous=word&15u;
        uint32_t expected[2][5]={{0}};
        begin(&e,previous);
        for (unsigned step=1;step<4;step++) {
            unsigned next=(word>>(4*step))&15u;
            if ((next^previous)&8u) {
                unsigned band=4;
                if ((next&7u)==(previous&7u)) {
                    for (unsigned i=0;i<4;i++) if ((next&7u)==refs[i]) band=i;
                }
                expected[next>>3][band]++;
            }
            hold(&e,next,5);
            for (unsigned s=0;s<2;s++) for (unsigned b=0;b<5;b++)
                assert(e.bins[s][b]==expected[s][b]);
            previous=next;
        }
    }
    /* Every band, either initial polarity: startup never invents an edge. */
    for (unsigned b=0;b<4;b++) for (unsigned initial=0;initial<2;initial++) {
        begin(&e, refs[b] | (initial<<3));
        assert(e.edges==0);
        hold(&e, refs[b] | ((1-initial)<<3),4);
        assert(e.edges==0);
        hold(&e, refs[b] | ((1-initial)<<3),1);
        assert(e.edges==1 && e.bins[1-initial][b]==1);
        hold(&e, refs[b] | (initial<<3),5);
        assert(e.edges==2 && e.bins[initial][b]==1);
    }
    /* Any reference disturbance during the DUT envelope makes it unknown. */
    begin(&e,1); hold(&e,9,2); hold(&e,11,1); hold(&e,9,2);
    assert(e.edges==1 && e.bins[1][4]==1);
    /* Even an already-qualified new reference at the DUT endpoint is ambiguous. */
    begin(&e,1); hold(&e,11,5);
    assert(e.bins[1][4]==1);
    /* Invalid thermometer codes do not suppress independent DUT observation. */
    for(unsigned ref=0;ref<8;ref++) if(ref!=0&&ref!=1&&ref!=3&&ref!=7) {
        begin(&e,ref); hold(&e,ref|8,5);
        assert(e.bins[1][4]==1 && e.invalid_ticks>0);
    }
    begin(&e,1); hold(&e,9,2); hold(&e,1,1);
    assert(e.noise==1 && e.edges==0);
    hold(&e,9,5); assert(e.bins[1][1]==1);
    begin(&e,0); hold(&e,1,5); assert(e.direction==1);
    hold(&e,3,5); assert(e.direction==1);
    hold(&e,1,5); assert(e.direction==2 && e.reversals==1);
    hold(&e,7,5); assert(e.direction==0 && e.skips==1);
    begin(&e,1); engine_sample(&e,now+2,9);
    assert(e.fault==F_GAP && e.edges==0 && e.gaps==1);
    hold(&e,9,20); assert(e.edges==0);
    begin(&e,1); e.noise=LIMIT; hold(&e,9,1); hold(&e,1,1);
    assert(e.fault==F_OVERFLOW && e.noise==LIMIT && e.edges==0);
    engine_init(&e); now=0xfffffffaU; hold(&e,1,10); hold(&e,9,5);
    assert(e.edges==1 && !e.fault);
    char frame[768]; assert(wire_encode(&e,frame,sizeof frame)>0);
    assert(wire_encode(&e,frame,2)==0);
    assert(wire_crc("123456789",9)==0x29b1);
    puts("PASS: all bands/polarities, boundary ambiguity, noise, topology, gap, overflow, wrap, CRC");
    return 0;
}
