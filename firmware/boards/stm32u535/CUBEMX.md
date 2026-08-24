# STM32CubeMX configuration - Sentry-Bee rev A

Target device: **STM32U535RET6**, LQFP64.

This is the authoritative CubeMX configuration checklist. Generate the STM32Cube project from these settings, then keep all Sentry-Bee application logic outside Cube-generated files.

## Project

- Toolchain: STM32CubeIDE / GCC.
- HAL enabled.
- ICACHE enabled.
- TrustZone: disabled for rev-A bring-up unless product security work explicitly enables it later.
- FreeRTOS: not required for initial bring-up; the application can remain event-driven/superloop until modem + sensor timing proves a scheduler is necessary.

## RCC / clocks

- LSE: crystal, 32.768 kHz.
- RTC clock source: LSE.
- HSE: disabled for rev A.
- SYSCLK target in performance profile: 160 MHz using internal oscillator + PLL according to CubeMX legal configuration.
- Low-power profiles may reduce SYSCLK when DSP is idle.
- ADF1 kernel clock must generate a clean PDM clock target of **2.048 MHz** for T5838 high-quality capture.
- OCTOSPI1 clock: start conservatively (<= 40 MHz) during bring-up; increase only after signal-integrity testing.

Do not hard-code PLL register values by hand: let the current CubeMX release validate voltage scaling, flash latency and peripheral kernel clocks.

## GPIO

### Outputs

- PC4 `MODEM_VBAT_EN`: push-pull, low speed, default LOW.
- PC5 `MODEM_PWRKEY_DRV`: push-pull, low speed, default LOW. This drives an external transistor; it does not connect directly to the modem PWRKEY pin.

### Inputs / EXTI

- PB2 `ACCEL_INT1`: external interrupt, rising edge, no internal pull unless required by the LIS2DW12 configuration.
- PC13 `SERVICE_BUTTON`: input/EXTI as needed, pull according to PCB button wiring.

## I2C1 - SHT40 + LIS2DW12

Pins:

- PB8 = I2C1_SCL
- PB9 = I2C1_SDA

Settings:

- Fast mode: 400 kHz.
- 7-bit addressing.
- analog filter enabled unless EMC tests require adjustment.
- external 4.7 kOhm pull-ups to 3V3.

Addresses used by firmware:

- SHT40: `0x44`.
- LIS2DW12 rev-A strap: `0x19`.

LIS2DW12 should use FIFO + INT1 in the production acquisition path. The current portable driver is sufficient for register bring-up but should not remain a per-sample polling implementation for long-term field firmware.

## USART1 - SIM7672E

Pins:

- PB6 = USART1_TX -> level translator -> modem RX.
- PB7 = USART1_RX <- level translator <- modem TX.

Initial settings:

- asynchronous;
- 115200 baud;
- 8 data bits;
- no parity;
- 1 stop bit;
- no hardware flow control in rev-A firmware bring-up.

The SIM7672X supports higher rates; stay at 115200 until the electrical path and parser are stable. Add RTS/CTS only if measurements show a need.

Use RX DMA + circular/ring buffering before production. The blocking line reader in `board_port.c` is deliberately a bring-up implementation.

## ADF1 - TDK T5838 PDM microphone

Pins:

- PB3 = ADF1_CCK0 -> microphone CLK.
- PB4 = ADF1_SDI0 <- microphone DATA.

Target acquisition:

- PDM clock: 2.048 MHz (T5838 high-quality mode).
- PCM target: 16 kHz mono.
- nominal decimation target: 128, or the closest legal ADF1 filter configuration yielding 16 kHz.
- signed PCM output via DMA.
- double buffer/ping-pong buffers; suggested starting block = 256 PCM samples.

Validate actual ADF1 filter/decimation parameters in CubeMX/reference manual; the invariant is the external PDM clock and final PCM rate, not a guessed register value.

For low-power experiments later, T5838 can be driven in its low-power clock range, but that is a second firmware profile and must not silently change dataset characteristics.

## OCTOSPI1 - W25Q128JV

Pins:

- PA3 = OCTOSPI1_CLK
- PA4 = OCTOSPI1_NCS
- PA6 = OCTOSPI1_IO3
- PA7 = OCTOSPI1_IO2
- PB0 = OCTOSPI1_IO1
- PB1 = OCTOSPI1_IO0

Initial mode:

- SDR;
- mode 0;
- indirect command mode during bring-up;
- 24-bit addressing for 16 MB device;
- conservative clock <=40 MHz;
- no memory-mapped execution requirement.

Implement JEDEC-ID read, sector erase, page program and readback test before using the flash as the offline queue/event store.

## ADC1 - battery monitor

- PA0 = ADC1_IN5.
- 12-bit conversion for the constants currently used by `sb_board_read_battery_v()`.
- long enough sample time for the high-impedance 1 MOhm / 330 kOhm divider plus 100 nF reservoir capacitor.
- use VDDA calibration/reference measurement if accurate battery voltage is required.

## RTC

- LSE clocked.
- 24-hour mode.
- UTC time stored in RTC; timezone belongs only in backend/UI.
- initialize from modem/network time at commissioning and periodically re-sync with sanity checks.

## USB (optional rev A service port)

- PA11 USB_DM.
- PA12 USB_DP.
- Device-only service/debug/DFU use.

If USB is omitted from the assembled PCB, retain test pads if routing cost is acceptable.

## SWD

- PA13 SWDIO.
- PA14 SWCLK.
- NRST on connector.
- never disable SWD in rev-A firmware.

## DMA priorities

Starting priority order:

1. ADF1 microphone DMA;
2. USART1 RX DMA;
3. OCTOSPI transfers;
4. other low-rate peripherals.

No long blocking critical sections are allowed while microphone acquisition is active.

## Low-power policy

The modem is a separately switched power domain. MCU STOP modes are allowed only after verifying:

- ADF/audio window is inactive or intentionally configured as wake source;
- I2C transaction complete;
- external flash idle;
- UART/modem state is known;
- RTC and accelerometer interrupt can wake the MCU as required.

## First generated project acceptance test

A generated Cube project is accepted only when it can, in this order:

1. boot and print firmware/build ID over debug path;
2. read SHT40 with valid CRC;
3. read LIS2DW12 WHO_AM_I = 0x44;
4. receive LIS2DW12 INT1;
5. read W25Q128 JEDEC ID and pass erase/program/readback test;
6. capture non-zero T5838 PCM samples over ADF1 DMA;
7. enable modem rail, issue PWRKEY, receive `OK` from `AT`;
8. read battery ADC;
9. enter/exit a low-power cycle and repeat all sensor checks.
