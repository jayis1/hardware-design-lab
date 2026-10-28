/* Author: jayis1. SPDX-License-Identifier: MIT */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#define PF_REGISTERS_H
static uint32_t tick, status, data;
#define USART_SR status
#define USART_DR data
#include "../firmware/drivers/serial.c"
uint32_t board_millis(void) { return tick; }
int main(void)
{
    assert(!serial_busy());
    assert(!serial_submit(0,1));
    assert(!serial_submit("",0));
    assert(!serial_submit("x",SERIAL_BUFFER+1));
    assert(serial_submit("abc",3));
    assert(!serial_submit("x",1));
    serial_poll(); assert(serial_busy() && data==0);
    status=1u<<7;
    serial_poll(); assert(data=='a');
    serial_poll(); assert(data=='b');
    serial_poll(); assert(data=='c' && !serial_busy());
    assert(!serial_take_timeout());
    tick=UINT32_MAX-50;
    assert(serial_submit("abc",3));
    status=0; tick+=99;
    serial_poll(); assert(serial_busy());
    ++tick; serial_poll();
    assert(!serial_busy() && serial_take_timeout());
    assert(!serial_take_timeout());
    assert(serial_submit("z",1)); status=1u<<7;
    serial_poll(); assert(data=='z' && !serial_busy());
    puts("PASS production UART bounds, busy rejection, nonblocking drain, wrap and timeout recovery");
}
