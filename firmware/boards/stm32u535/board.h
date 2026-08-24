#ifndef SENTRY_BEE_BOARD_STM32U535_H
#define SENTRY_BEE_BOARD_STM32U535_H

#include <stdint.h>
#include <stdbool.h>
#include "sentry_bee/platform.h"
#include "stm32u5xx_hal.h"

/* Frozen rev-A GPIO assignments. Keep synchronized with hardware/PINOUT.md. */
#define SB_MODEM_VBAT_EN_PORT   GPIOC
#define SB_MODEM_VBAT_EN_PIN    GPIO_PIN_4
#define SB_MODEM_PWRKEY_PORT    GPIOC
#define SB_MODEM_PWRKEY_PIN     GPIO_PIN_5
#define SB_ACCEL_INT1_PORT      GPIOB
#define SB_ACCEL_INT1_PIN       GPIO_PIN_2
#define SB_SERVICE_BUTTON_PORT  GPIOC
#define SB_SERVICE_BUTTON_PIN   GPIO_PIN_13

/* SIM7672X timing constants from the hardware design guide. */
#define SB_MODEM_PWRKEY_ON_MS        100U
#define SB_MODEM_PWRKEY_OFF_MS      2600U
#define SB_MODEM_VBAT_SETTLE_MS       20U
#define SB_MODEM_BOOT_GUARD_MS       500U
#define SB_MODEM_OFF_ON_GUARD_MS    2100U

extern I2C_HandleTypeDef hi2c1;
extern UART_HandleTypeDef huart1;
extern RTC_HandleTypeDef hrtc;
extern ADC_HandleTypeDef hadc1;

extern const sb_platform_t g_sb_stm32u535_platform;

void sb_board_gpio_safe_state(void);
void sb_board_modem_vbat(bool on);
void sb_board_modem_pwrkey_pulse(uint32_t pulse_ms);
void sb_board_modem_power_on(void);
void sb_board_modem_force_off(void);
float sb_board_read_battery_v(void);
uint64_t sb_board_unix_time(void);

#endif
