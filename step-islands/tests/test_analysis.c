/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include <assert.h>
#include <stdio.h>
/* Independent all-pairs distance closure; production uses flood fill/BFS. */
int main(void)
{
    for (unsigned history=0; history<65536; ++history) {
        Engine e; engine_init(&e); e.settings=(uint16_t)history;
        unsigned distance[16][16], labels[16], count=0;
        for (unsigned a=0; a<16; ++a) {
            labels[a]=16;
            for (unsigned b=0; b<16; ++b) {
                unsigned difference=a^b;
                bool edge=difference && !(difference&(difference-1));
                distance[a][b]=((history>>a)&(history>>b)&1u) ?
                    (a==b?0u:(edge?1u:99u)):99u;
            }
        }
        for (unsigned k=0; k<16; ++k)
            for (unsigned a=0; a<16; ++a)
                for (unsigned b=0; b<16; ++b)
                    if (distance[a][k]+distance[k][b]<distance[a][b])
                        distance[a][b]=distance[a][k]+distance[k][b];
        for (unsigned a=0; a<16; ++a) {
            if (!(history&(1u<<a))) continue;
            for (unsigned b=0; b<16; ++b)
                if (distance[a][b]<99) { labels[a]=b; break; }
            if (labels[a]==a) ++count;
        }
        assert(engine_components(&e)==count);
        unsigned bridges=0;
        for (unsigned a=0; a<16; ++a) {
            if (history&(1u<<a)) continue;
            unsigned groups=0;
            for (unsigned bit=0; bit<4; ++bit) {
                unsigned n=a^(1u<<bit);
                if (labels[n]<16) groups|=1u<<labels[n];
            }
            if (engine_popcount(groups)>=2) bridges|=1u<<a;
        }
        assert(engine_bridges(&e)==bridges);
        /* Every start, one rotating target per set, plus all targets on
         * representative sparse/dense sets: endpoints and edge witnesses. */
        for (unsigned a=0; a<16; ++a) {
            for (unsigned b=0; b<16; ++b) {
                if (history%257 && b!=(history%16)) continue;
                uint8_t path[16];
                unsigned length=engine_path(&e,a,b,path);
                assert(length==(distance[a][b]==99?0:distance[a][b]+1));
                if (!length) continue;
                assert(path[0]==a && path[length-1]==b);
                for (unsigned i=0; i<length; ++i) {
                    assert(history&(1u<<path[i]));
                    if (i) assert(engine_popcount(path[i]^path[i-1])==1);
                }
            }
        }
    }
    Engine e; engine_init(&e); uint8_t path[16];
    assert(!engine_path(&e,16,0,path));
    assert(!engine_path(&e,0,0,NULL));
    e.settings=(1u<<0)|(1u<<3);
    assert(engine_components(&e)==2 && engine_bridges(&e)==6);
    e.settings|=1u<<1;
    assert(engine_components(&e)==1 && engine_bridges(&e)==0);
    assert(engine_path(&e,0,3,path)==3 && path[1]==1);
    puts("PASS: 65536 histories, independent distance/component/bridge oracle and path witnesses");
}
