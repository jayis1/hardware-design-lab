/* Author: jayis1. SPDX-License-Identifier: MIT */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
/* Replace only memory-mapped registers; execute the production UART driver. */
#define RC_REGISTERS_H
static uint32_t status, data, now;
#define USART_SR status
#define USART_DR data
#include "board.h"
uint32_t board_millis(void) { return now; }
#include "../firmware/drivers/serial.c"
int main(void)
{
    assert(!serial_submit(NULL, 1));
    assert(!serial_submit("", 0));
    assert(!serial_submit("x", SERIAL_BUFFER + 1));
    assert(serial_submit("abc",3));
    assert(!serial_submit("x",1));
    serial_poll(); assert(serial_busy());
    status = 1u << 7;
    serial_poll(); assert(data=='a');
    serial_poll(); assert(data=='b');
    serial_poll(); assert(data=='c' && !serial_busy());
    assert(!serial_take_timeout());
    now = UINT32_MAX - 10;
    assert(serial_submit("d",1));
    status=0; now+=99; serial_poll(); assert(serial_busy());
    ++now; serial_poll(); assert(!serial_busy());
    assert(serial_take_timeout()); assert(!serial_take_timeout());
    status=1u<<7; assert(serial_submit("e",1)); serial_poll();
    assert(data=='e' && !serial_busy());
    puts("PASS production UART bounds, backpressure, timeout, wrap, recovery");
}
