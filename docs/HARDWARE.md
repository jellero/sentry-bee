# Hardware design v1

## Core components

### MCU — STM32U535

Use a STM32U535 variant with enough flash for secure boot/OTA growth; 512 KB is the current design target. Required peripherals:

- I2C for SHT40 and LIS2DW12 during prototype stage;
- preferably SPI for LIS2DW12 on the production board if bus robustness/throughput warrants it;
- PDM/SAI/DFSDM-capable path for T5838 microphone capture;
- UART with DMA for SIM7672E;
- QSPI/OSPI for external NOR;
- RTC/LPTIM for low-power scheduling;
- SWD test pads.

## LTE — SIM7672E

Use the exact EU LTE Cat-1 bis SKU and verify supported bands with the distributor/manufacturer before PCB freeze. The application requires LTE operation and does not depend on GSM/2G fallback.

### Power integrity

Do not power the modem from a weak MCU rail. Give it a dedicated high-current buck/rail with low-ESR bulk capacitance placed according to the SIMCom hardware design guide. LTE current pulses determine regulator sizing and PCB copper, not average consumption.

Expose:

- PWRKEY control from MCU;
- hardware power/load switch or regulator enable;
- UART TX/RX;
- modem status/network status if available;
- SIM/eSIM interface;
- antenna connector plus ESD network and RF keep-out.

The board must be able to hard power-cycle the modem when the AT interface becomes unrecoverable.

## Vibration — LIS2DW12

Prototype address assumes SA0=1 / I2C address 0x19. WHO_AM_I is checked before configuration.

Current firmware configures 800 Hz high-performance acquisition. The sensor must be mechanically coupled to the hive. Do not place it only on the main enclosure if that enclosure is foam/tape isolated from the hive.

Recommended physical implementation:

- 10-20 mm rigid sensor daughterboard;
- two mounting holes or a stiff bonded mechanical interface;
- short cable/FPC to main PCB;
- defined orientation marking;
- repeatable mounting position across hives.

Mechanical repeatability is part of the measurement system.

## Temperature/RH — SHT40

I2C address 0x44. The firmware uses high-precision command 0xFD and validates both CRC bytes.

Placement rules:

- expose sensor to hive air but protect it from direct bee contact, propolis and liquid water;
- isolate it thermally from the modem/regulator/main PCB;
- avoid a sealed enclosure around the sensor;
- use a replaceable small sensor board if contamination is expected.

## Acoustic — TDK T5838

Use PDM into an STM32-compatible digital microphone input. Acoustic port design matters: use a protected acoustic vent/membrane and avoid placing the microphone behind thick enclosure walls.

Production firmware should use DMA ping-pong buffers. Raw audio should not be transmitted continuously; only features and event clips are retained.

## Local flash

16-32 MB QSPI NOR is sufficient for telemetry queue, crash logs and a useful amount of event data. Partition concept:

- 10% metadata/config/logs;
- 20% telemetry store-and-forward queue;
- 70% circular event/raw-data storage.

Add CRC per record and a monotonic sequence number so power loss cannot corrupt the queue.

## Power system

Recommended field architecture:

```text
solar panel -> charger/power-path -> LiFePO4 battery
                                |-> 3.3 V low-Iq rail -> STM32 + sensors
                                +-> modem rail -> SIM7672E
```

LiFePO4 is preferred for outdoor cycle life and thermal safety. Final panel/battery size must be calculated from measured LTE attach/upload energy at the target apiary, not modem datasheet idle current.

## Environmental protection

- main enclosure: target IP65/IP67;
- conformal coat main PCB, excluding microphone/SHT40 sensing areas, RF connectors and required contacts;
- TVS on external power and long sensor cables;
- reverse-polarity and input surge protection;
- hydrophobic vent to reduce condensation pressure cycles;
- antenna outside conductive enclosures.

## PCB bring-up test points

Mandatory pads:

- 3V3, modem VBAT, battery input, ground;
- SWDIO/SWCLK/NRST;
- UART modem TX/RX;
- I2C SDA/SCL;
- PWRKEY/modem enable;
- microphone clock/data;
- QSPI signals on first revision where space allows.
