# STM32U535 rev-A board port

This directory is the hardware adaptation layer for the first Sentry-Bee custom PCB.

## What is implemented

- frozen GPIO definitions in `board.h`;
- I2C1 callbacks for SHT40 and LIS2DW12;
- USART1 callbacks for SIM7672E AT traffic;
- RTC-to-Unix timestamp conversion;
- battery ADC conversion for the rev-A divider;
- modem VBAT switch and PWRKEY sequencing;
- complete CubeMX peripheral checklist in `CUBEMX.md`.

The portable application still lives in `firmware/src` and does not depend on STM32 HAL.

## Required generated Cube handles

`board_port.c` expects these Cube-generated global handles:

```c
I2C_HandleTypeDef hi2c1;
UART_HandleTypeDef huart1;
RTC_HandleTypeDef hrtc;
ADC_HandleTypeDef hadc1;
```

ADF1 and OCTOSPI handles are intentionally not referenced by the portable platform API yet. They belong to board-specific high-throughput capture/storage modules and should use DMA/non-blocking paths.

## Integration into generated `main.c`

After Cube peripheral initialization and before enabling application interrupts/tasks:

```c
#include "board.h"
#include "sentry_bee/sentry_bee.h"

sb_board_gpio_safe_state();

const sb_platform_t *platform = &g_sb_stm32u535_platform;

float t = 0.0f, rh = 0.0f;
if (!sb_sht40_read(platform, &t, &rh)) {
    Error_Handler();
}

if (!sb_lis2dw12_init_800hz_fifo(platform, 16U)) {
    Error_Handler();
}
```

Do not paste Sentry-Bee feature/anomaly logic into Cube `USER CODE` blocks. Build it as separate application sources so Cube regeneration cannot destroy it.

## Accelerometer interrupt path

PB2 is EXTI from LIS2DW12 INT1. ISR/callback must only set a flag or notify a task; it must not perform I2C transfers inside the interrupt.

Example:

```c
static volatile bool g_accel_fifo_ready;

void HAL_GPIO_EXTI_Rising_Callback(uint16_t pin) {
    if (pin == SB_ACCEL_INT1_PIN) {
        g_accel_fifo_ready = true;
    }
}
```

Main/task context then drains the FIFO:

```c
int16_t fifo[32][3];
if (g_accel_fifo_ready) {
    g_accel_fifo_ready = false;
    size_t n = sb_lis2dw12_read_fifo(platform, fifo, 32U);
    /* Convert selected/mechanical axis to g, append to DSP window. */
}
```

## Vibration scaling

The initial portable driver returns raw 16-bit register words. The signal-processing path must convert them according to the selected LIS2DW12 operating mode/full-scale and must document which mechanical axis is used. Do not train a model on undocumented raw orientation.

## Audio path

Production target:

```text
T5838 PDM -> ADF1 -> DMA ping/pong -> signed PCM 16 kHz -> feature window
```

The first profile uses a 2.048 MHz PDM clock in T5838 high-quality mode and 16 kHz PCM output. The audio DMA callback should only swap/queue buffers; FFT/band feature calculation runs outside the ISR.

Keep a compile-time dataset mode that stores short PCM clips around anomalies. Normal field firmware should store/transmit derived features instead of continuous audio.

## External flash path

Production target:

```text
W25Q128JV <-> OCTOSPI1 <-> append-only event store / offline telemetry queue
```

Bring-up order:

1. read JEDEC ID;
2. erase one sacrificial sector;
3. program deterministic pattern;
4. read/CRC verify;
5. power-cycle and verify persistence;
6. only then enable the telemetry queue.

Do not make filesystem correctness a dependency for first field tests. A small append-only record log with CRC and sequence numbers is preferred initially.

## Modem lifecycle

Normal connection cycle:

1. `sb_board_modem_power_on()`;
2. repeat basic `AT` until responsive;
3. SIM ready check;
4. wait for LTE registration;
5. publish queued MQTT/TLS records;
6. request graceful modem shutdown (`AT+CPOF`) or enter PSM according to field profile;
7. once shutdown is confirmed, disable the 3V8 modem rail if using full power cycling.

`sb_board_modem_force_off()` is recovery behavior, not the normal shutdown path.

## UART implementation status

The current `board_port.c` line reader is deliberately blocking because it is useful for first PCB bring-up. Before field firmware, replace it with:

- USART1 RX DMA circular buffer;
- lock-free/ring parser input;
- AT command state machine with URC handling;
- command deadlines without blocking sensor acquisition.

## Build separation

Host CI continues to build the portable core. The STM32 board directory is built only inside the generated Cube project because it requires the STM32U5 HAL/CMSIS package.

A second CI job should be added once the generated Cube project/toolchain files are checked in or reproducibly fetched.
