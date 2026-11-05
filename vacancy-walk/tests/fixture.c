/* Author: jayis1. SPDX-License-Identifier: MIT */
#include <assert.h>
#include <stdio.h>
#include "engine.h"
#include "wire.h"
static void emit(Engine *e)
{
    char data[512];
    size_t length=wire_encode(e,data,sizeof data);
    assert(length);
    assert(fwrite(data,1,length,stdout)==length);
}
static void run(Engine *e, unsigned mask, unsigned count)
{
    while(count--) {
        engine_sample(e,e->now+1,mask);
        emit(e);
    }
}
int main(void)
{
    /* Synthetic trace: two moves, one cancellation, ambiguous extra lift. */
    Engine e; engine_init(&e);
    run(&e,14,60);
    run(&e,12,5); run(&e,13,55);
    run(&e,9,5); run(&e,11,55);
    run(&e,9,5); run(&e,11,55);
    run(&e,3,5); run(&e,1,5);
    assert(e.moves==2 && e.canceled==1 && e.abandoned==1);
}
