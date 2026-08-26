# Product-v2 main-board I/O requirements

The product-v2 STM32 pinout is **not frozen yet**. Do not reuse the rev-A direct-sensor pin assignment blindly.

## Required interfaces

| Interface | Requirement | Notes |
|---|---|---|
| LTE UART | TX/RX, DMA preferred | SIM7672E through 1.8/3.3-V translator |
| RS-485 | UART TX/RX + DE/RE GPIO | Modbus RTU master to hive probes |
| Audio | ADC input + DMA + timer trigger | analog audio from 5-m shielded cable; anti-alias filter at board input |
| QSPI/OSPI | 1x quad NOR | event/queue storage |
| Battery ADC | 1 channel | divider + calibration |
| Sensor-rail control | 1 GPIO | enables switched 12-V boost/load path |
| Modem rail control | 1 GPIO | controls modem load switch |
| Modem PWRKEY | 1 GPIO through transistor | no direct 3.3-V drive to modem PWRKEY |
| RTC/LSE | 32.768-kHz crystal | low-power scheduling |
| SWD | SWDIO/SWCLK/NRST | mandatory |
| Service | optional USB or UART pads | manufacturing/recovery |

## Product-v2 cable connector signals

Minimum field interface:

```text
RS485_A
RS485_B
+12V_SENSOR
GND_SENSOR
AUDIO
AUDIO_RETURN
SHIELD / connector shell strategy
```

If balanced audio is adopted, the third pair becomes `AUDIO+ / AUDIO-`.

## Placement constraints

- RS-485 TVS and field connector at board edge.
- 12-V boost switching node away from audio input.
- audio filter/receiver close to field connector, then route as quiet analog to ADC.
- modem/RF area isolated from audio front end.
- 3.8-V modem bulk capacitance close to modem VBAT.
- QSPI traces short and compact.

## Pin-freeze gate

Freeze exact STM32U535RET6 pins only after these are selected:

1. exact audio topology (single-ended vs balanced receiver);
2. exact external flash candidate after BY25Q128 validation;
3. service interface policy;
4. final RS-485 transceiver package;
5. exact solar/charger control/telemetry requirements.

Until then, `hardware/PINOUT.md` is explicitly the rev-A R&D/reference pinout only.
