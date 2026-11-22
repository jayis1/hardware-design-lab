/* Author: jayis1. SPDX-License-Identifier: MIT */
#include <stdio.h>
#include "engine.h"
#include "wire.h"
int main(void)
{
    Engine e; engine_init(&e);
    char line[768];
    const unsigned masks[]={15,14,15,7,15,14,15,13,0,11,15};
    for(unsigned j=0;j<sizeof masks/sizeof masks[0];++j) {
        unsigned duration=j==10?1100:100;
        for(unsigned t=0;t<duration;++t) {
            engine_sample(&e,e.now+1,masks[j]);
            size_t n=wire_encode(&e,line,sizeof line);
            if(!n) return 1;
            if(fwrite(line,1,n,stdout)!=n) return 1;
        }
    }
    return 0;
}
