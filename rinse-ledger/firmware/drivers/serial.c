/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "board.h"
#include "registers.h"
static char pending[SERIAL_BUFFER];
static size_t length;
static size_t position;
static uint32_t submitted;
static bool timeout;
bool serial_busy(void)
{
    return position < length;
}
bool serial_submit(const char *data, size_t size)
{
    if (!data || !size || size > sizeof pending || serial_busy()) {
        return false;
    }
    for (size_t i = 0; i < size; ++i) {
        pending[i] = data[i];
    }
    length = size;
    position = 0;
    submitted = board_millis();
    return true;
}
void serial_poll(void)
{
    if (!serial_busy()) {
        return;
    }
    if (board_millis() - submitted >= 100u) {
        position = length;
        timeout = true;
        return;
    }
    if (USART_SR & (1u << 7)) {
        USART_DR = (uint8_t)pending[position++];
    }
}
bool serial_take_timeout(void)
{
    bool result = timeout;
    timeout = false;
    return result;
}
