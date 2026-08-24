#include "sentry_bee/sentry_bee.h"
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define LIS_ADDR 0x19U
#define SHT_ADDR 0x44U

static uint8_t g_writes[16][8];
static size_t g_write_len[16];
static uint8_t g_write_addr[16];
static size_t g_write_count;
static uint8_t g_fifo_samples;
static unsigned g_xyz_index;
static bool g_sht_pending;

static uint8_t sht_crc(const uint8_t *d, size_t n) {
    uint8_t crc = 0xFFU;
    for (size_t i = 0; i < n; ++i) {
        crc ^= d[i];
        for (unsigned b = 0; b < 8U; ++b) {
            crc = (crc & 0x80U) ? (uint8_t)((crc << 1) ^ 0x31U)
                                : (uint8_t)(crc << 1);
        }
    }
    return crc;
}

static void reset_mock(void) {
    memset(g_writes, 0, sizeof(g_writes));
    memset(g_write_len, 0, sizeof(g_write_len));
    memset(g_write_addr, 0, sizeof(g_write_addr));
    g_write_count = 0U;
    g_fifo_samples = 0U;
    g_xyz_index = 0U;
    g_sht_pending = false;
}

static bool mock_i2c_write(uint8_t addr7, const uint8_t *data, size_t len) {
    if (!data || len == 0U || g_write_count >= 16U || len > sizeof(g_writes[0])) return false;
    g_write_addr[g_write_count] = addr7;
    g_write_len[g_write_count] = len;
    memcpy(g_writes[g_write_count], data, len);
    ++g_write_count;
    if (addr7 == SHT_ADDR && len == 1U && data[0] == 0xFDU) g_sht_pending = true;
    return true;
}

static bool mock_i2c_read(uint8_t addr7, uint8_t *data, size_t len) {
    if (addr7 != SHT_ADDR || !g_sht_pending || !data || len != 6U) return false;

    /* 25 C and 50 %RH-like deterministic raw values. */
    const uint16_t raw_t = 0x6666U;
    const uint16_t raw_rh = 0x72B0U;
    data[0] = (uint8_t)(raw_t >> 8);
    data[1] = (uint8_t)raw_t;
    data[2] = sht_crc(data, 2U);
    data[3] = (uint8_t)(raw_rh >> 8);
    data[4] = (uint8_t)raw_rh;
    data[5] = sht_crc(&data[3], 2U);
    g_sht_pending = false;
    return true;
}

static bool mock_i2c_write_read(uint8_t addr7,
                                const uint8_t *tx, size_t tx_len,
                                uint8_t *rx, size_t rx_len) {
    if (addr7 != LIS_ADDR || !tx || tx_len != 1U || !rx) return false;

    if (tx[0] == 0x0FU && rx_len == 1U) {
        rx[0] = 0x44U;
        return true;
    }

    if (tx[0] == 0x2FU && rx_len == 1U) {
        rx[0] = g_fifo_samples;
        return true;
    }

    if (tx[0] == 0x28U && rx_len == 6U) {
        const int16_t base = (int16_t)(100 + (int)g_xyz_index * 10);
        const int16_t values[3] = {base, (int16_t)(base + 1), (int16_t)(base + 2)};
        for (unsigned axis = 0U; axis < 3U; ++axis) {
            rx[axis * 2U] = (uint8_t)((uint16_t)values[axis] & 0xFFU);
            rx[axis * 2U + 1U] = (uint8_t)((uint16_t)values[axis] >> 8);
        }
        ++g_xyz_index;
        return true;
    }

    return false;
}

static void mock_delay(uint32_t ms) { (void)ms; }

static const sb_platform_t g_platform = {
    .i2c_write = mock_i2c_write,
    .i2c_read = mock_i2c_read,
    .i2c_write_read = mock_i2c_write_read,
    .delay_ms = mock_delay,
};

static void test_sht40_crc_path(void) {
    reset_mock();
    float t = 0.0f;
    float rh = 0.0f;
    assert(sb_sht40_read(&g_platform, &t, &rh));
    assert(t > 24.0f && t < 26.0f);
    assert(rh > 49.0f && rh < 51.0f);
    assert(g_write_count == 1U);
    assert(g_write_addr[0] == SHT_ADDR);
    assert(g_writes[0][0] == 0xFDU);
}

static void test_lis_fifo_configuration(void) {
    reset_mock();
    assert(sb_lis2dw12_init_800hz_fifo(&g_platform, 16U));
    assert(g_write_count == 5U);

    /* CTRL1: 800 Hz high-performance. */
    assert(g_writes[0][0] == 0x20U && g_writes[0][1] == 0x84U);
    /* CTRL2: BDU + automatic address increment. */
    assert(g_writes[1][0] == 0x21U && g_writes[1][1] == 0x0CU);
    /* CTRL6: +/-2g default filtering. */
    assert(g_writes[2][0] == 0x25U && g_writes[2][1] == 0x00U);
    /* FIFO continuous mode + threshold 16. */
    assert(g_writes[3][0] == 0x2EU && g_writes[3][1] == 0xD0U);
    /* FIFO threshold routed to INT1. */
    assert(g_writes[4][0] == 0x23U && g_writes[4][1] == 0x02U);

    reset_mock();
    assert(!sb_lis2dw12_init_800hz_fifo(&g_platform, 0U));
    reset_mock();
    assert(!sb_lis2dw12_init_800hz_fifo(&g_platform, 32U));
}

static void test_lis_fifo_read(void) {
    reset_mock();
    g_fifo_samples = 0x03U;
    uint8_t count = 0U;
    bool overrun = true;
    assert(sb_lis2dw12_fifo_count(&g_platform, &count, &overrun));
    assert(count == 3U);
    assert(!overrun);

    int16_t xyz[4][3] = {{0}};
    const size_t n = sb_lis2dw12_read_fifo(&g_platform, xyz, 4U);
    assert(n == 3U);
    assert(xyz[0][0] == 100 && xyz[0][1] == 101 && xyz[0][2] == 102);
    assert(xyz[1][0] == 110 && xyz[1][1] == 111 && xyz[1][2] == 112);
    assert(xyz[2][0] == 120 && xyz[2][1] == 121 && xyz[2][2] == 122);

    g_fifo_samples = 0x45U; /* five unread samples + overrun flag */
    assert(sb_lis2dw12_fifo_count(&g_platform, &count, &overrun));
    assert(count == 5U);
    assert(overrun);
}

int main(void) {
    test_sht40_crc_path();
    test_lis_fifo_configuration();
    test_lis_fifo_read();
    puts("driver tests: OK");
    return 0;
}
