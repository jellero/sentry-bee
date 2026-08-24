#include "board.h"
#include <stddef.h>
#include <string.h>

#define SB_I2C_TIMEOUT_MS   100U
#define SB_UART_TX_TIMEOUT 1000U

static bool board_i2c_write(uint8_t addr7, const uint8_t *data, size_t len) {
    if (!data || len == 0U || len > 0xFFFFU) return false;
    return HAL_I2C_Master_Transmit(&hi2c1, (uint16_t)(addr7 << 1),
                                   (uint8_t *)data, (uint16_t)len,
                                   SB_I2C_TIMEOUT_MS) == HAL_OK;
}

static bool board_i2c_read(uint8_t addr7, uint8_t *data, size_t len) {
    if (!data || len == 0U || len > 0xFFFFU) return false;
    return HAL_I2C_Master_Receive(&hi2c1, (uint16_t)(addr7 << 1), data,
                                  (uint16_t)len, SB_I2C_TIMEOUT_MS) == HAL_OK;
}

static bool board_i2c_write_read(uint8_t addr7,
                                 const uint8_t *tx, size_t tx_len,
                                 uint8_t *rx, size_t rx_len) {
    if (!tx || tx_len == 0U || !rx || rx_len == 0U ||
        tx_len > 0xFFFFU || rx_len > 0xFFFFU) return false;

    if (HAL_I2C_Master_Transmit(&hi2c1, (uint16_t)(addr7 << 1),
                                (uint8_t *)tx, (uint16_t)tx_len,
                                SB_I2C_TIMEOUT_MS) != HAL_OK) return false;

    return HAL_I2C_Master_Receive(&hi2c1, (uint16_t)(addr7 << 1), rx,
                                  (uint16_t)rx_len,
                                  SB_I2C_TIMEOUT_MS) == HAL_OK;
}

static bool board_uart_write(const uint8_t *data, size_t len) {
    if (!data || len == 0U || len > 0xFFFFU) return false;
    return HAL_UART_Transmit(&huart1, (uint8_t *)data, (uint16_t)len,
                             SB_UART_TX_TIMEOUT) == HAL_OK;
}

static int board_uart_read_line(char *dst, size_t dst_len, uint32_t timeout_ms) {
    if (!dst || dst_len < 2U) return -1;

    const uint32_t start = HAL_GetTick();
    size_t used = 0U;

    while ((HAL_GetTick() - start) < timeout_ms && used < dst_len - 1U) {
        uint8_t c = 0U;
        if (HAL_UART_Receive(&huart1, &c, 1U, 20U) != HAL_OK) continue;
        dst[used++] = (char)c;
        if (c == '\n') break;
    }

    dst[used] = '\0';
    return used > 0U ? (int)used : 0;
}

static void board_delay_ms(uint32_t ms) {
    HAL_Delay(ms);
}

void sb_board_gpio_safe_state(void) {
    HAL_GPIO_WritePin(SB_MODEM_VBAT_EN_PORT, SB_MODEM_VBAT_EN_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(SB_MODEM_PWRKEY_PORT, SB_MODEM_PWRKEY_PIN, GPIO_PIN_RESET);
}

void sb_board_modem_vbat(bool on) {
    HAL_GPIO_WritePin(SB_MODEM_VBAT_EN_PORT, SB_MODEM_VBAT_EN_PIN,
                      on ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void sb_board_modem_pwrkey_pulse(uint32_t pulse_ms) {
    HAL_GPIO_WritePin(SB_MODEM_PWRKEY_PORT, SB_MODEM_PWRKEY_PIN, GPIO_PIN_SET);
    HAL_Delay(pulse_ms);
    HAL_GPIO_WritePin(SB_MODEM_PWRKEY_PORT, SB_MODEM_PWRKEY_PIN, GPIO_PIN_RESET);
}

void sb_board_modem_power_on(void) {
    sb_board_modem_vbat(true);
    HAL_Delay(SB_MODEM_VBAT_SETTLE_MS);
    sb_board_modem_pwrkey_pulse(SB_MODEM_PWRKEY_ON_MS);
    HAL_Delay(SB_MODEM_BOOT_GUARD_MS);
}

void sb_board_modem_force_off(void) {
    /* Prefer AT+CPOF in normal operation. This path is a controlled fallback. */
    sb_board_modem_pwrkey_pulse(SB_MODEM_PWRKEY_OFF_MS);
    HAL_Delay(SB_MODEM_OFF_ON_GUARD_MS);
    sb_board_modem_vbat(false);
}

static void board_modem_power(bool on) {
    if (on) sb_board_modem_power_on();
    else sb_board_modem_force_off();
}

float sb_board_read_battery_v(void) {
    if (HAL_ADC_Start(&hadc1) != HAL_OK) return 0.0f;
    if (HAL_ADC_PollForConversion(&hadc1, 20U) != HAL_OK) {
        (void)HAL_ADC_Stop(&hadc1);
        return 0.0f;
    }

    const uint32_t raw = HAL_ADC_GetValue(&hadc1);
    (void)HAL_ADC_Stop(&hadc1);

    /* Rev-A divider: 1.0 MOhm high side, 330 kOhm low side.
       ADC must be configured 12-bit. Calibrate VDDA/divider in production. */
    const float adc_v = ((float)raw * 3.3f) / 4095.0f;
    return adc_v * (1330.0f / 330.0f);
}

static int64_t days_from_civil(int y, unsigned m, unsigned d) {
    y -= (m <= 2U) ? 1 : 0;
    const int era = (y >= 0 ? y : y - 399) / 400;
    const unsigned yoe = (unsigned)(y - era * 400);
    const int mi = (int)m;
    const unsigned mp = (unsigned)(mi + (mi > 2 ? -3 : 9));
    const unsigned doy = (153U * mp + 2U) / 5U + d - 1U;
    const unsigned doe = yoe * 365U + yoe / 4U - yoe / 100U + doy;
    return (int64_t)era * 146097LL + (int64_t)doe - 719468LL;
}

uint64_t sb_board_unix_time(void) {
    RTC_TimeTypeDef t = {0};
    RTC_DateTypeDef d = {0};

    /* Read time before date as required by STM32 RTC shadow-register behavior. */
    if (HAL_RTC_GetTime(&hrtc, &t, RTC_FORMAT_BIN) != HAL_OK) return 0U;
    if (HAL_RTC_GetDate(&hrtc, &d, RTC_FORMAT_BIN) != HAL_OK) return 0U;

    const int year = 2000 + (int)d.Year;
    const int64_t days = days_from_civil(year, d.Month, d.Date);
    if (days < 0) return 0U;

    return (uint64_t)days * 86400ULL +
           (uint64_t)t.Hours * 3600ULL +
           (uint64_t)t.Minutes * 60ULL +
           (uint64_t)t.Seconds;
}

const sb_platform_t g_sb_stm32u535_platform = {
    .i2c_write = board_i2c_write,
    .i2c_read = board_i2c_read,
    .i2c_write_read = board_i2c_write_read,
    .uart_write = board_uart_write,
    .uart_read_line = board_uart_read_line,
    .delay_ms = board_delay_ms,
    .unix_time = sb_board_unix_time,
    .modem_power = board_modem_power,
};
