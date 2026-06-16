// error_handler.h — Error Handler Header
#ifndef ERROR_HANDLER_H
#define ERROR_HANDLER_H
#include <stdint.h>

#define ERROR_LOG_SIZE      64
#define ERROR_STRING_MAX    48

typedef struct {
    uint32_t timestamp;
    uint8_t  error_code;
    int16_t  detail;
    char     message[ERROR_STRING_MAX];
} error_entry_t;

void error_handler_init(void);
void error_handler_record(uint8_t code, int16_t detail, const char *msg);
int  error_handler_get_count(void);
int  error_handler_get_entry(uint32_t index, error_entry_t *entry);
void error_handler_clear(void);
const char *error_handler_code_string(uint8_t code);
#endif
