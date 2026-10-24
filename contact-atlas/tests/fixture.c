/* Author: jayis1. Synthetic stimulus, not a physical measurement. */
#include "engine.h"
#include "wire.h"
#include <stdio.h>
static Atlas a;
int main(void)
{
    uint32_t sequence=0;
    char out[256];
    atlas_init(&a,0);
    for (uint32_t t=1;t<=1000;t++) {
        uint8_t mask=t<300 ? 1 : (t<310 ? 0 : (t<600 ? 2 : (t<607 ? 3 : 1)));
        atlas_sample(&a,t,mask);
        if (t%100==0) {
            size_t n=wire_status(out,sizeof out,&a,t,sequence++,0);
            fwrite(out,1,n,stdout);
        }
        AtlasEvent event;
        while (atlas_pop(&a,&event)) {
            size_t n=wire_event(out,sizeof out,&event,sequence++);
            fwrite(out,1,n,stdout);
        }
    }
}
