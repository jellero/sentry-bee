# Sentry-Bee v1 pinout

Target MCU: **STM32U535RET6**, LQFP64, 512 KB flash, LDO variant.

The goal is to keep the first custom board routable, debuggable and cheap while preserving all required functions: LTE, PDM audio, vibration, temperature/RH, external event storage and battery telemetry.

## Frozen pin assignment

| MCU pin | Peripheral | Signal | Connected device | Notes |
|---|---|---|---|---|
| PA0 | ADC1_IN5 | BAT_SENSE | battery divider | 1 MOhm / 330 kOhm divider; add 100 nF at ADC input |
| PA3 | OCTOSPI1_CLK | FLASH_CLK | W25Q128JV | external event storage |
| PA4 | OCTOSPI1_NCS | FLASH_CS | W25Q128JV | pull-up 47 kOhm |
| PA6 | OCTOSPI1_IO3 | FLASH_IO3 | W25Q128JV | quad mode |
| PA7 | OCTOSPI1_IO2 | FLASH_IO2 | W25Q128JV | quad mode |
| PB0 | OCTOSPI1_IO1 | FLASH_IO1 | W25Q128JV | quad mode |
| PB1 | OCTOSPI1_IO0 | FLASH_IO0 | W25Q128JV | quad mode |
| PB2 | GPIO/EXTI | ACCEL_INT1 | LIS2DW12 INT1 | wake/threshold/FIFO interrupt |
| PB3 | ADF1_CCK0 | MIC_CLK | TDK T5838 | PDM clock output |
| PB4 | ADF1_SDI0 | MIC_DATA | TDK T5838 | PDM data input |
| PB6 | USART1_TX | MODEM_RX | TXU0202 -> SIM7672E | 3.3 V to 1.8 V level translation |
| PB7 | USART1_RX | MODEM_TX | TXU0202 <- SIM7672E | 1.8 V to 3.3 V level translation |
| PB8 | I2C1_SCL | I2C_SCL | SHT40 + LIS2DW12 | 4.7 kOhm pull-up to 3.3 V |
| PB9 | I2C1_SDA | I2C_SDA | SHT40 + LIS2DW12 | 4.7 kOhm pull-up to 3.3 V |
| PC4 | GPIO output | MODEM_VBAT_EN | modem load switch | active high |
| PC5 | GPIO output | MODEM_PWRKEY_DRV | N-MOS/NPN pull-down | MCU never drives PWRKEY directly |
| PA11 | USB_DM | USB_DM | service connector | optional prototype/service port |
| PA12 | USB_DP | USB_DP | service connector | optional prototype/service port |
| PA13 | SWDIO | SWDIO | Tag-Connect/SWD | keep dedicated |
| PA14 | SWCLK | SWCLK | Tag-Connect/SWD | keep dedicated |
| PC13 | GPIO input | SERVICE_BUTTON | pushbutton/test pad | optional manual wake/commissioning |

## Why ADF1 is used for the microphone

Using `PB1/MDF1_SDI0` for PDM would collide with `PB1/OCTOSPI1_IO0`. The microphone is therefore assigned to `PB3/ADF1_CCK0` and `PB4/ADF1_SDI0`, leaving all six low-bank OCTOSPI signals available.

## Sensor bus policy

SHT40 and LIS2DW12 share I2C1 at 400 kHz. The LIS2DW12 is operated with FIFO + INT1 rather than polling every sample. This keeps bus occupancy and MCU wakeups low even at high ODR.

## LTE UART electrical domain

SIM7672X main UART is a **1.8 V logic domain**. USART1 must not be wired directly to the 3.3 V STM32 pins. Use a dual-supply, opposite-direction fixed translator such as **TXU0202**:

- VCCA = SIM7672E VDD_EXT (1.8 V)
- VCCB = 3V3
- channel A->B = modem TX to MCU RX
- channel B->A = MCU TX to modem RX
- OE enabled only when desired; the device must support partial-power-down/high-Z when one rail disappears.

## Debug policy

SWD is mandatory on every board revision. Do not reclaim PA13/PA14. USB is optional for the prototype but useful for manufacturing and field recovery.

## Clock plan

- LSE: 32.768 kHz crystal for RTC and long-term scheduling.
- HSE: not required for v1; use internal oscillators/PLL.
- high-performance mode: up to 160 MHz only during DSP, flash operations and bursts of work.
- low-power mode: reduce system clock aggressively between acquisition windows.

The exact CubeMX clock tree must keep ADF1 and OCTOSPI kernel clocks inside the device limits and must be validated on the final PCB.
