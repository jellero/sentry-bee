#include "sentry_bee/sentry_bee.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define SHT40_ADDR 0x44U
#define LIS2DW12_ADDR 0x19U
#define LIS2DW12_WHO_AM_I 0x0FU
#define LIS2DW12_CTRL1 0x20U
#define LIS2DW12_CTRL2 0x21U
#define LIS2DW12_CTRL4_INT1_PAD_CTRL 0x23U
#define LIS2DW12_CTRL6 0x25U
#define LIS2DW12_OUT_X_L 0x28U
#define LIS2DW12_FIFO_CTRL 0x2EU
#define LIS2DW12_FIFO_SAMPLES 0x2FU

static uint8_t crc8_sht(const uint8_t *d, size_t n) {
    uint8_t crc = 0xFFU;
    for (size_t i = 0; i < n; ++i) {
        crc ^= d[i];
        for (int b = 0; b < 8; ++b) crc = (crc & 0x80U) ? (uint8_t)((crc << 1) ^ 0x31U) : (uint8_t)(crc << 1);
    }
    return crc;
}

static bool lis_write_reg(const sb_platform_t *p, uint8_t reg, uint8_t value) {
    uint8_t b[2] = {reg, value};
    return p && p->i2c_write && p->i2c_write(LIS2DW12_ADDR, b, sizeof(b));
}

static bool lis_read_reg(const sb_platform_t *p, uint8_t reg, uint8_t *value) {
    return p && p->i2c_write_read && value &&
           p->i2c_write_read(LIS2DW12_ADDR, &reg, 1U, value, 1U);
}

bool sb_sht40_read(const sb_platform_t *p, float *temperature_c, float *humidity_rh) {
    if (!p || !p->i2c_write || !p->i2c_read || !p->delay_ms || !temperature_c || !humidity_rh) return false;
    uint8_t cmd = 0xFDU;
    uint8_t r[6];
    if (!p->i2c_write(SHT40_ADDR, &cmd, 1)) return false;
    p->delay_ms(10);
    if (!p->i2c_read(SHT40_ADDR, r, sizeof(r))) return false;
    if (crc8_sht(r, 2) != r[2] || crc8_sht(&r[3], 2) != r[5]) return false;
    uint16_t st = (uint16_t)((r[0] << 8) | r[1]);
    uint16_t srh = (uint16_t)((r[3] << 8) | r[4]);
    *temperature_c = -45.0f + 175.0f * ((float)st / 65535.0f);
    *humidity_rh = -6.0f + 125.0f * ((float)srh / 65535.0f);
    if (*humidity_rh < 0.0f) *humidity_rh = 0.0f;
    if (*humidity_rh > 100.0f) *humidity_rh = 100.0f;
    return true;
}

bool sb_lis2dw12_init_800hz(const sb_platform_t *p) {
    if (!p || !p->i2c_write_read || !p->i2c_write) return false;
    uint8_t who = 0U;
    if (!lis_read_reg(p, LIS2DW12_WHO_AM_I, &who) || who != 0x44U) return false;

    /* ODR=800 Hz; high-performance mode as used by the initial vibration model. */
    if (!lis_write_reg(p, LIS2DW12_CTRL1, 0x84U)) return false;
    /* BDU=1, IF_ADD_INC=1 for coherent multi-byte reads. */
    if (!lis_write_reg(p, LIS2DW12_CTRL2, 0x0CU)) return false;
    /* +/-2 g, default filtering. */
    return lis_write_reg(p, LIS2DW12_CTRL6, 0x00U);
}

bool sb_lis2dw12_init_800hz_fifo(const sb_platform_t *p, uint8_t threshold_samples) {
    if (!sb_lis2dw12_init_800hz(p)) return false;
    if (threshold_samples == 0U || threshold_samples > 31U) return false;

    /* Continuous FIFO mode (FMode=110) + programmable FTH[4:0]. */
    const uint8_t fifo_ctrl = (uint8_t)(0xC0U | (threshold_samples & 0x1FU));
    if (!lis_write_reg(p, LIS2DW12_FIFO_CTRL, fifo_ctrl)) return false;

    /* Route FIFO threshold to INT1. Other INT1 sources remain disabled. */
    return lis_write_reg(p, LIS2DW12_CTRL4_INT1_PAD_CTRL, 0x02U);
}

bool sb_lis2dw12_read_xyz(const sb_platform_t *p, int16_t xyz[3]) {
    if (!p || !p->i2c_write_read || !xyz) return false;
    uint8_t reg = LIS2DW12_OUT_X_L;
    uint8_t r[6];
    if (!p->i2c_write_read(LIS2DW12_ADDR, &reg, 1, r, sizeof(r))) return false;
    for (int i = 0; i < 3; ++i) xyz[i] = (int16_t)((uint16_t)r[i * 2] | ((uint16_t)r[i * 2 + 1] << 8));
    return true;
}

bool sb_lis2dw12_fifo_count(const sb_platform_t *p, uint8_t *count, bool *overrun) {
    if (!count || !overrun) return false;
    uint8_t s = 0U;
    if (!lis_read_reg(p, LIS2DW12_FIFO_SAMPLES, &s)) return false;
    *count = (uint8_t)(s & 0x3FU);
    *overrun = (s & 0x40U) != 0U;
    return true;
}

size_t sb_lis2dw12_read_fifo(const sb_platform_t *p, int16_t (*xyz)[3], size_t capacity) {
    if (!p || !xyz || capacity == 0U) return 0U;

    uint8_t count = 0U;
    bool overrun = false;
    if (!sb_lis2dw12_fifo_count(p, &count, &overrun)) return 0U;
    (void)overrun; /* Caller can query it separately for diagnostics. */

    size_t n = count;
    if (n > capacity) n = capacity;
    for (size_t i = 0U; i < n; ++i) {
        if (!sb_lis2dw12_read_xyz(p, xyz[i])) return i;
    }
    return n;
}

static bool at_expect(const sb_platform_t *p, const char *cmd, const char *needle, uint32_t timeout_ms) {
    if (!p || !p->uart_write || !p->uart_read_line) return false;
    char tx[96];
    int n = snprintf(tx, sizeof(tx), "%s\r\n", cmd);
    if (n <= 0 || !p->uart_write((const uint8_t *)tx, (size_t)n)) return false;
    char line[160];
    uint32_t remaining = timeout_ms;
    while (remaining > 0U) {
        int got = p->uart_read_line(line, sizeof(line), remaining > 1000U ? 1000U : remaining);
        if (got > 0 && strstr(line, needle)) return true;
        if (got > 0 && strstr(line, "ERROR")) return false;
        remaining = remaining > 1000U ? remaining - 1000U : 0U;
    }
    return false;
}

bool sb_sim7672_basic_check(const sb_platform_t *p) {
    return at_expect(p, "AT", "OK", 3000U) &&
           at_expect(p, "ATE0", "OK", 3000U) &&
           at_expect(p, "AT+CPIN?", "READY", 5000U);
}

bool sb_sim7672_wait_network(const sb_platform_t *p, uint32_t timeout_ms) {
    if (!p || !p->uart_write || !p->uart_read_line) return false;
    uint32_t remaining = timeout_ms;
    while (remaining > 0U) {
        const char cmd[] = "AT+CEREG?\r\n";
        if (!p->uart_write((const uint8_t *)cmd, sizeof(cmd) - 1U)) return false;
        char line[160];
        int got = p->uart_read_line(line, sizeof(line), 2000U);
        if (got > 0 && (strstr(line, ",1") || strstr(line, ",5"))) return true;
        if (p->delay_ms) p->delay_ms(1000U);
        remaining = remaining > 3000U ? remaining - 3000U : 0U;
    }
    return false;
}

bool sb_sim7672_get_rssi(const sb_platform_t *p, int16_t *rssi_dbm) {
    if (!p || !p->uart_write || !p->uart_read_line || !rssi_dbm) return false;
    const char cmd[] = "AT+CSQ\r\n";
    if (!p->uart_write((const uint8_t *)cmd, sizeof(cmd) - 1U)) return false;
    char line[160];
    for (int i = 0; i < 4; ++i) {
        if (p->uart_read_line(line, sizeof(line), 1000U) <= 0) continue;
        char *q = strstr(line, "+CSQ:");
        if (!q) continue;
        int csq = atoi(q + 5);
        if (csq < 0 || csq > 31) return false;
        *rssi_dbm = (int16_t)(-113 + 2 * csq);
        return true;
    }
    return false;
}
