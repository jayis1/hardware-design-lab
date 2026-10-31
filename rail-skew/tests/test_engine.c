/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include "wire.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
static void hold(Engine *e, unsigned mask, unsigned count)
{
    while (count--) {
        engine_sample(e, e->time + 1, mask);
        assert(engine_valid(e));
    }
}
static void emit(Engine *e)
{
    char out[512];
    size_t n=wire_encode(e,out,sizeof out);
    assert(n); fputs(out,stdout);
}
static void fixture(void)
{
    Engine e; engine_init(&e);
    unsigned masks[]={0,1,3,7,15,5,0};
    for(unsigned i=0;i<7;i++) { hold(&e,masks[i],100); emit(&e); }
}
int main(int argc, char **argv)
{
    if(argc>1 && !strcmp(argv[1],"fixture")) { fixture(); return 0; }
    Engine e; engine_init(&e);
    hold(&e,0,5); assert(e.state==READY);
    hold(&e,1,5); assert(e.state==ACTIVE && e.starts==1);
    hold(&e,1,20); assert(e.bins[1]==20);
    hold(&e,3,5); hold(&e,3,20); assert(e.bins[2]==20);
    hold(&e,7,5); hold(&e,15,5);
    assert(e.completed[0]==1 && e.last_peak==2 && e.state==READY);
    hold(&e,5,10); hold(&e,0,5);
    assert(e.completed[1]==1 && e.last_peak==0);
    /* A return to origin is an abandoned excursion, not a completed pass. */
    hold(&e,1,5); hold(&e,0,5); assert(e.aborted==1);
    /* Per-rail qualification: right chatter cannot delay left progress. */
    engine_init(&e); hold(&e,0,5);
    for(unsigned i=0;i<5;i++) hold(&e,1|((i%2)*4),1);
    assert(e.state==ACTIVE && (e.levels&3)==1);
    /* Invalid thermometer code must persist to qualify; raw glitches lose coverage. */
    engine_init(&e); hold(&e,0,5); hold(&e,1,5);
    uint32_t bin=e.bins[1]; hold(&e,2,1); hold(&e,1,1);
    assert(e.bins[1]==bin); hold(&e,1,1); assert(e.bins[1]==bin+1);
    hold(&e,2,5); assert(e.state==HALTED && e.aborted==1 && (e.flags&F_CODE));
    /* Gap abandons once, freezes accounting and does not invent time. */
    engine_init(&e); hold(&e,0,5); hold(&e,1,10);
    engine_sample(&e,e.time+2,1); assert(engine_valid(&e));
    assert(e.state==HALTED && e.aborted==1 && e.gaps==1);
    hold(&e,15,100); assert(e.completed[0]==0 && e.aborted==1);
    /* Exact timeout takes precedence over arrival. */
    engine_init(&e); hold(&e,0,5); hold(&e,1,5);
    hold(&e,1,MAX_TRAVEL_MS-5); hold(&e,15,5);
    assert(e.aborted==1 && !e.completed[0] && e.state==READY);
    /* Direct endpoint change lacks an intermediate observation. */
    engine_init(&e); hold(&e,0,5); hold(&e,15,5);
    assert(e.starts==1 && e.aborted==1 && e.state==READY);
    /* Ordinary timestamp wrap remains consecutive. */
    engine_init(&e); e.time=UINT32_MAX-3; hold(&e,0,10); assert(e.state==READY);
    /* Transaction rollback reserves abandonment without partial bin growth. */
    engine_init(&e); hold(&e,0,5); hold(&e,1,5);
    e.bins[1]=UINT32_MAX; hold(&e,1,1);
    assert(e.state==HALTED && e.aborted==1 && e.bins[1]==UINT32_MAX);
    engine_init(&e); e.starts=UINT32_MAX; e.aborted=UINT32_MAX;
    hold(&e,0,5); hold(&e,1,5); assert(e.state==HALTED && e.starts==UINT32_MAX);
    /* All 9 legal stage pairs, in both directions, use an independent difference oracle. */
    unsigned code[]={0,1,3};
    for(unsigned direction=0;direction<2;direction++)
        for(unsigned l=0;l<3;l++) for(unsigned r=0;r<3;r++) {
            unsigned mask=code[l]|(code[r]<<2);
            if(mask==0 || mask==15) continue;
            engine_init(&e); hold(&e,direction?15:0,5); hold(&e,mask,5);
            assert(e.state==ACTIVE);
            unsigned d=l>r?l-r:r-l;
            hold(&e,mask,17);
            assert(e.bins[3*direction+d]==17 && e.peak==d);
            hold(&e,direction?0:15,5);
            assert(e.completed[direction]==1 && e.last_peak==d);
        }
    /* Every legal pair followed by every raw vector, for both origins.
     * This checks independently bundled qualification and every terminal
     * path against the encoder's evidence contract. */
    for(unsigned origin=0;origin<2;origin++)
        for(unsigned l=0;l<3;l++) for(unsigned r=0;r<3;r++)
            for(unsigned next=0;next<16;next++) {
                engine_init(&e); hold(&e,origin?15:0,5);
                hold(&e,code[l]|(code[r]<<2),5);
                hold(&e,next,5);
                char record[512];
                assert(wire_encode(&e,record,sizeof record));
                if((next&3)==2 || (next>>2)==2)
                    assert(e.state==HALTED && (e.flags&F_CODE));
            }
    char out[512]; engine_init(&e);
    assert(wire_crc("123456789",9)==0x29b1);
    assert(!wire_encode(&e,out,4));
    e.last_peak=3; assert(!wire_encode(&e,out,sizeof out));
    puts("PASS engine: traversal, nine stage pairs, faults, boundaries, overflow, CRC");
    return 0;
}
