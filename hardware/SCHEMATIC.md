# Sentry-Bee rev-A R&D/reference schematic design rules

> **Status:** R&D/reference board. This document freezes the earlier direct-sensor electrical architecture used for lab acquisition and comparison. It is **not** the current product-v2 field architecture. See `docs/PRODUCT_ARCHITECTURE.md`, `docs/HARDWARE.md`, `hardware/BOM.csv` and `hardware/COSTING.md` for the current direction using RS-485 T/RH + vibration probes and analog audio over cable.

This reference board remains intentionally specific enough to reproduce a known acquisition platform for sensor correlation testing.

## 1. MCU

Use **STM32U535RET6**, LQFP64, 512 KB Flash, non-SMPS/LDO package variant.

Required support circuitry:

- 100 nF local decoupling at every VDD pin;
- one local bulk capacitor in the 4.7-10 uF range near the MCU power cluster;
- VDDA filtered from 3V3 with ferrite/0-ohm option plus 100 nF + 1 uF local decoupling;
- VCAP exactly per the current STM32U535 datasheet/reference schematic;
- NRST exposed on SWD/Tag-Connect;
- BOOT configuration/test pad retained;
- 32.768 kHz LSE crystal with load capacitors selected from the chosen crystal CL and measured PCB parasitics.

Do not copy capacitor values for VCAP/LSE from memory: take them from the selected crystal and the current ST reference design during schematic review.

## 2. Main power tree

Recommended first prototype battery: **1-cell LiFePO4** for field safety and cycle life.

Because the SIM7672E requires VBAT in the 3.4-4.2 V range, a LiFePO4 cell must **not** feed the modem directly.

```text
LiFePO4 cell
    |
    +-- charger / solar front end
    |
    +-- 3V3 low-Iq regulator ----------> STM32 + sensors + flash + mic
    |
    +-- 3V8 buck-boost, >=1.2 A transient capability
            |
            +-- controlled load switch ---> SIM7672E VBAT pins
```

SIM7672X documentation specifies 3.4-4.2 V, 3.8 V nominal and a documented peak current around 746 mA. Design the 3V8 rail for margin, transient response and cold-battery operation; target at least **1.2 A transient capability** rather than sizing exactly to the nominal peak.

At the modem VBAT pins:

- short, wide copper pour;
- bulk capacitance close to pins; start with 100 uF low-ESR plus 10 uF + 1 uF + 100 nF ceramic network;
- verify the exact capacitance/ESR recommendations against the current SIM7672X Hardware Design revision;
- do not route modem RF return current through MCU/sensor ground necks.

## 3. Modem power control

Use two separate controls:

1. `MODEM_VBAT_EN`: controls a high-side load switch on 3V8.
2. `MODEM_PWRKEY_DRV`: drives a small transistor that pulls SIM7672E PWRKEY low.

Never drive PWRKEY directly from a 3.3 V MCU output. The modem PWRKEY input is not a 3.3 V GPIO domain.

Power-on sequence:

1. enable 3V8 load switch;
2. wait for rail settling;
3. pulse PWRKEY low through the transistor for about 50-100 ms;
4. wait for STATUS/UART readiness;
5. send `AT` until the modem responds.

Normal shutdown:

1. request graceful shutdown using the supported AT command (`AT+CPOF`) or the documented long PWRKEY pulse;
2. wait until the modem is actually off;
3. only then remove VBAT with the load switch.

Do not hard-cut VBAT as the normal shutdown method because modem flash integrity can be affected.

## 4. UART level translation

SIM7672X MAIN_UART is 1.8 V logic. Add **TXU0202** or an equivalent fixed-direction dual-supply translator with one channel each direction and partial-power-down isolation.

```text
STM32 PB6 / USART1_TX (3V3) --> TXU0202 --> SIM7672 MAIN_UART_RXD (1V8)
STM32 PB7 / USART1_RX (3V3) <-- TXU0202 <-- SIM7672 MAIN_UART_TXD (1V8)
```

Translator rails:

- 3V3 side from the sensor/MCU rail;
- 1V8 side from SIM7672E `VDD_EXT`;
- 100 nF at each translator supply pin;
- OE default state must not back-power the modem.

Route RTS/CTS pads to test points even if rev-A firmware starts with TX/RX only. If sustained large transfers later need hardware flow control, they can be enabled in a later reference revision without changing the modem footprint.

## 5. SIM interface

Use nano-SIM socket for prototypes. eSIM can be considered after carrier selection.

Requirements:

- ESD array specifically suitable for SIM lines;
- keep SIM traces short and away from antenna/feedline;
- follow SIMCom reference topology for series/ESD components;
- expose SIM_DET only if the selected socket supports it.

## 6. LTE RF

The PCB must support the European low bands, especially B20/B28 as required by the selected SIM7672E SKU/operator.

For rev A prefer a **u.FL/I-PEX connector** and an external qualified LTE antenna. This avoids spending rev A on custom antenna tuning.

RF rules:

- 50 ohm controlled-impedance feed;
- Pi matching footprint close to the modem RF pin, initially DNP/0-ohm as appropriate;
- no high-speed digital routing under the RF feed;
- maintain the modem vendor keep-out and ground-via recommendations;
- provide a conducted RF test option if practical.

## 7. Vibration sensor

LIS2DW12 is physically separated from the main electronics if the enclosure is mechanically isolated from the hive.

Recommended implementation:

- tiny 2-layer daughterboard with LIS2DW12;
- VDD/VDDIO = 3V3;
- local 100 nF and bulk capacitor per ST reference circuit;
- I2C1 + INT1 connection over a short cable;
- ESD protection if the cable exits the electronics enclosure;
- rigid screw/adhesive mechanical coupling to a repeatable hive structural point.

The daughterboard placement must be documented for every field unit; otherwise vibration datasets are not comparable.

## 8. Temperature / humidity

SHT40 is on a remote or thermally isolated sensing tongue/daughterboard, not beside the modem regulator.

- I2C address 0x44;
- VDD 3V3;
- local 100 nF;
- protected from propolis/water while allowing air exchange;
- cable length kept modest; for long harnesses reduce I2C edge rate and validate EMC.

## 9. PDM microphone

TDK T5838:

- clock from PB3 / ADF1_CCK0;
- data to PB4 / ADF1_SDI0;
- local decoupling as required by TDK;
- acoustic port must not be blocked by conformal coating, gasket or enclosure wall;
- add an acoustic membrane if required for humidity/dust protection;
- keep PDM lines away from LTE RF feed and buck-boost switching node.

## 10. External event storage

Use **W25Q128JV (16 MB)** or equivalent qualified 3V3 QSPI NOR for the reference board.

OCTOSPI1 mapping is frozen in `PINOUT.md`.

Storage budget is intentionally event-oriented, not continuous recording. Firmware stores:

- rolling telemetry queue;
- short vibration windows around anomalies;
- short audio-derived/raw clips only when enabled for dataset collection;
- crash/diagnostic logs;
- configuration backup.

## 11. Battery measurement

PA0 / ADC1_IN5 reads the cell through a high-value divider.

Starting values:

- top: 1.0 MOhm;
- bottom: 330 kOhm;
- 100 nF from ADC input to ground.

Calibrate divider ratio in firmware per PCB batch if battery state estimation depends on absolute voltage. For LiFePO4, voltage alone is not an accurate state-of-charge estimator over the flat part of the discharge curve; treat it as safety/health telemetry unless a coulomb counter is later added.

## 12. Service/debug

Mandatory pads/connector:

- SWDIO;
- SWCLK;
- NRST;
- GND;
- 3V3 reference;
- modem TX/RX test pads;
- MODEM_VBAT_EN;
- MODEM_PWRKEY_DRV;
- LIS2DW12 INT1;
- LTE 3V8 rail measurement pad.

Optional USB D+/D- on PA11/PA12 is recommended on rev A for factory/service use.

## 13. PCB partitioning

Keep four physical zones:

1. RF/modem;
2. 3V8 switching power;
3. MCU/digital/flash;
4. low-noise sensors/audio connectors.

The buck-boost inductor/switch node must be physically distant from the microphone and vibration sensor connector. Use an uninterrupted ground plane where RF/layout guidance allows and stitch ground aggressively around noisy power/RF boundaries.
