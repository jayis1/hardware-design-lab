// error_handler.c — Error Handler Implementation
// Circular log buffer for runtime error tracking
// Stores timestamp, error code, detail, and message for diagnostics

#include "error_handler.h"
#include "../registers.h"
#include "../board.h"
#include <string.h>

static error_entry_t error_log[ERROR_LOG_SIZE];
static uint32_t error_count = 0;
static uint32_t error_head = 0;  // Next write position
static bool initialized = false;

void error_handler_init(void) {
    memset(error_log, 0, sizeof(error_log));
    error_count = 0;
    error_head = 0;
    initialized = true;
}

void error_handler_record(uint8_t code, int16_t detail, const char *msg) {
    if (!initialized) return;

    error_entry_t *entry = &error_log[error_head];

    entry->timestamp = REG_READ(SYS_UPTIME_LO_REG);
    entry->error_code = code;
    entry->detail = detail;

    if (msg) {
        strncpy(entry->message, msg, ERROR_STRING_MAX - 1);
        entry->message[ERROR_STRING_MAX - 1] = '\0';
    } else {
        entry->message[0] = '\0';
    }

    error_head = (error_head + 1) % ERROR_LOG_SIZE;
    if (error_count < ERROR_LOG_SIZE) {
        error_count++;
    }
}

int error_handler_get_count(void) {
    return error_count;
}

int error_handler_get_entry(uint32_t index, error_entry_t *entry) {
    if (!initialized) return -1;
    if (index >= error_count) return -2;

    // Entries are stored circularly; index 0 is the oldest
    uint32_t oldest = (error_count < ERROR_LOG_SIZE)
                    ? 0
                    : error_head;  // head points to next write = oldest when full
    uint32_t actual_index = (oldest + index) % ERROR_LOG_SIZE;

    memcpy(entry, &error_log[actual_index], sizeof(error_entry_t));
    return 0;
}

void error_handler_clear(void) {
    memset(error_log, 0, sizeof(error_log));
    error_count = 0;
    error_head = 0;
}

const char *error_handler_code_string(uint8_t code) {
    switch (code) {
        case ERR_NONE:             return "No error";
        case ERR_BUSY:             return "Device busy";
        case ERR_TIMEOUT:          return "Operation timed out";
        case ERR_OVERFLOW:         return "Buffer overflow";
        case ERR_PARAM:            return "Invalid parameter";
        case ERR_STATE:            return "Invalid state";
        case ERR_HARDWARE:         return "Hardware fault";
        case ERR_CRC:              return "CRC check failed";
        case ERR_OVERTEMP:         return "Over temperature";
        case ERR_NOT_CALIBRATED:   return "Not calibrated";
        case ERR_UNKNOWN:          return "Unknown error";
        default:                   return "Undefined error";
    }
}

// Dump all errors to a buffer for USB transmission
int error_handler_dump(uint8_t *buffer, uint32_t max_len, uint32_t *written) {
    if (!initialized) return -1;

    uint32_t offset = 0;
    for (uint32_t i = 0; i < error_count && offset < max_len; i++) {
        error_entry_t entry;
        error_handler_get_entry(i, &entry);

        // Format: [timestamp:4][code:1][detail:2][msg_len:1][msg:N]
        if (offset + 8 > max_len) break;

        buffer[offset++] = (entry.timestamp >> 0) & 0xFF;
        buffer[offset++] = (entry.timestamp >> 8) & 0xFF;
        buffer[offset++] = (entry.timestamp >> 16) & 0xFF;
        buffer[offset++] = (entry.timestamp >> 24) & 0xFF;
        buffer[offset++] = entry.error_code;
        buffer[offset++] = (entry.detail >> 0) & 0xFF;
        buffer[offset++] = (entry.detail >> 8) & 0xFF;

        uint8_t msg_len = strlen(entry.message);
        if (offset + 1 + msg_len > max_len) {
            msg_len = max_len - offset - 1;
        }
        buffer[offset++] = msg_len;
        memcpy(buffer + offset, entry.message, msg_len);
        offset += msg_len;
    }

    *written = offset;
    return 0;
}
