/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include "wire.h"
#include <assert.h>
#include <stdio.h>
static void emit(const Engine *e)
{
    char line[768];
    size_t n=wire_encode(e,line,sizeof line);
    assert(n); fwrite(line,1,n,stdout);
}
static void hold(Engine *e, unsigned raw, unsigned ticks)
{
    while (ticks--) {
        engine_sample(e,e->now+1,(uint8_t)raw);
        if (e->now%100u==0) emit(e);
    }
    emit(e);
}
int main(void)
{
    Engine e; engine_init(&e); emit(&e);
    hold(&e,15,105); /* all pockets present, lid closed */
    hold(&e,7,5); hold(&e,6,5); hold(&e,7,5); hold(&e,15,105);
    hold(&e,7,5); hold(&e,3,5); hold(&e,11,105); /* leave C absent */
    hold(&e,3,5); hold(&e,7,5); hold(&e,15,105); /* add C back */
    engine_sample(&e,e.now+2,15); emit(&e); /* deliberate acquisition gap */
    hold(&e,15,100);
    return 0;
}
