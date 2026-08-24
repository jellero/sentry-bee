#ifndef SENTRY_BEE_PLATFORM_H
#define SENTRY_BEE_PLATFORM_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    bool (*i2c_write)(uint8_t addr7, const uint8_t *data, size_t len);
    bool (*i2c_read)(uint8_t addr7, uint8_t *data, size_t len);
    bool (*i2c_write_read)(uint8_t addr7, const uint8_t *tx, size_t tx_len,
                           uint8_t *rx, size_t rx_len);
    bool (*uart_write)(const uint8_t *data, size_t len);
    int  (*uart_read_line)(char *dst, size_t dst_len, uint32_t timeout_ms);
    void (*delay_ms)(uint32_t ms);
    uint64_t (*unix_time)(void);
    void (*modem_power)(bool on);
} sb_platform_t;

#endif
