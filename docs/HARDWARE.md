# Hardware design

## Product-v2 field architecture

The preferred product architecture uses one external main unit and simple hive-side sensors over a shielded cable up to approximately 5 m.

```text
HIVE SIDE                               MAIN UNIT

RS-485 T/RH probe ---------------------> RS-485
RS-485 vibration probe ----------------> STM32U535
analog microphone + preamp -----------> ADC/DMA + DSP
                                         QSPI
                                         SIM7672E LTE
                                         LiFePO4 + solar
```

The older direct-sensor STM32 board using LIS2DW12/SHT40/T5838 is retained as an R&D/reference platform. Its pinout and schematic files remain in `hardware/` for correlation testing.

## MCU — STM32U535RET6

The current main-board target is STM32U535RET6, LQFP64, 512 KB.

Product-v2 required peripherals:

- UART or equivalent serial peripheral for RS-485/Modbus RTU;
- ADC + DMA for analog audio;
- UART with DMA for SIM7672E;
- QSPI/OSPI for external NOR;
- RTC/LPTIM for low-power scheduling;
- ADC for battery/rail monitoring;
- SWD test pads.

The product-v2 main board no longer requires long external I2C/SPI/PDM sensor wiring.

## LTE — SIM7672E

Use the exact EU LTE Cat-1 bis SKU and verify bands before PCB/order freeze. The application requires LTE operation and does not depend on GSM/2G fallback.

### LTE power integrity

Give the modem a dedicated 3.8-V high-current rail with low-ESR bulk capacitance and layout derived from the current SIMCom hardware design guide. LTE current pulses determine regulator sizing and PCB copper.

Expose/control:

- PWRKEY;
- hardware load switch/regulator enable;
- UART TX/RX through 1.8-V/3.3-V translation;
- modem status if useful;
- SIM/eSIM interface;
- antenna connector and RF keep-out.

The main board must be able to recover from an unresponsive modem, while normal shutdown remains graceful before VBAT removal.

## RS-485 field bus

Use a half-duplex RS-485 transceiver and Modbus RTU master on the STM32.

Main-unit cable entry should include:

- RS-485 TVS such as SM712-class protection;
- deliberate termination and bias arrangement;
- connector/chassis strategy that does not route surge current through sensitive analog ground;
- test points for A/B and transceiver enable.

Start with conservative baud rate. A 5-m cable does not require high speed for T/RH and vibration telemetry.

### Ready-made T/RH probe

Use a commercial RS-485/Modbus temperature/RH probe when it passes:

- register-map verification;
- accuracy/response test against a reference SHT40;
- condensation test;
- propolis/bee-contact protection review;
- power-cycle and brownout behavior test.

The low-cost probe in `hardware/BOM.csv` is a validation candidate, not yet an approved production part.

### Ready-made vibration probe

The product should not accept a vibration sensor that exposes only one aggregate RMS/velocity value.

Preferred information:

- raw acceleration windows; or
- dominant/spectral frequencies with amplitudes; or
- configurable band energies; or
- at minimum dominant frequency plus RMS/peak-type metrics.

Validate the commercial probe side-by-side with the LIS2DW12 reference hardware on the same mechanical location.

Mechanical repeatability remains part of the measurement system: mounting orientation, stiffness and position must be documented.

## Acoustic path — analog over cable

Audio does not use RS-485.

The microphone sits near/in the hive and is amplified locally. The preamp output then travels over the shielded cable to the main-unit ADC.

Preferred prototype topology:

```text
analog microphone -> low-noise preamp -> shielded pair -> anti-alias filter -> STM32 ADC/DMA
```

Do not run a raw microphone capsule signal 5 m before amplification.

If single-ended audio shows LTE/switcher pickup, migrate the cable pair to a balanced line driver/receiver while keeping the microphone/preamp local.

The microphone acoustic port must be protected against liquid water/contamination without sealing away the hive sound.

## External cable

Initial target: shielded 6-conductor / three-pair cable, 5 m maximum design length.

Suggested pair allocation:

- pair 1: RS485-A / RS485-B;
- pair 2: +12V_SENSOR / GND;
- pair 3: AUDIO / AUDIO_RETURN, or AUDIO+ / AUDIO- if balanced;
- shield: EMC/chassis termination at the main unit, with footprints/options to tune the final strategy during testing.

Use a sealed connector; exact M8/M12 vs automotive sealed connector remains TBD until cable OD/current/assembly cost are frozen.

## Local flash

A 128-Mbit QSPI NOR is adequate for telemetry queue, logs and event clips. The low-cost BY25Q128 candidate must be validated against the known W25Q128 reference before production selection.

Recommended partition concept:

- metadata/config/logs;
- telemetry store-and-forward queue;
- circular event/audio storage.

Each record should include CRC and monotonic sequence metadata so unexpected power loss cannot silently corrupt the queue.

## Power system

Keep a **1S LiFePO4** battery as the main energy store.

```text
solar panel -> charger/power path -> 1S LiFePO4
                                |-> 3.3 V low-Iq -> STM32 + flash + logic
                                |-> 3.8 V high-current -> SIM7672E
                                +-> switched 12 V boost -> RS-485 probes
```

The 12-V rail exists only for external field probes. It should normally be disabled when probes can tolerate duty-cycled operation.

Do not assume duty cycling is compatible with every vibration probe: if the selected unit needs continuous internal sampling/spectral estimation, include its continuous power in the winter solar budget.

Final battery and panel size must be based on measured:

- LTE attach/upload energy;
- sensor/probe consumption and warm-up;
- audio acquisition duty cycle;
- converter efficiency at actual load;
- winter solar availability at the target installation.

## Environmental protection

- main enclosure: target IP65/IP67;
- external probes: target IP65/IP67 or protected mounting;
- conformal coat main PCB except connectors/test contacts where inappropriate;
- TVS at external power/data entry;
- reverse-polarity/input protection;
- hydrophobic enclosure vent where needed;
- antenna outside conductive enclosure;
- strain relief on the 5-m field cable.

## PCB bring-up test points

Mandatory pads for the product-v2 main board:

- battery, 3V3, 3V8 modem, switched 12V sensor rail, GND;
- SWDIO/SWCLK/NRST;
- modem UART TX/RX;
- RS485 TX/RX direction logic and A/B near transceiver;
- modem PWRKEY/load-switch control;
- audio input after cable receiver/preamp interface;
- QSPI signals on the first revision where practical.

## Costing

See `hardware/BOM.csv` and `hardware/COSTING.md`. Low-cost marketplace probes remain `VALIDATE` until they pass electrical, mechanical and field-data checks.
