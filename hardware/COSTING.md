# Sentry-Bee cost snapshot

This document records the low-volume component price snapshot used for the product-v2 architecture discussion. It is not a purchase order and it is not a guaranteed quotation.

Snapshot date: **2026-08-24**.

Prices exclude shipping unless explicitly included. VAT treatment depends on vendor/account. Marketplace parts are validation candidates, not approved production components.

## Cost strategy

The product direction deliberately buys difficult field sensors as ready-made RS-485/Modbus devices and keeps the custom electronics concentrated in one external main unit:

- temperature/humidity: ready-made RS-485 probe;
- vibration: ready-made RS-485 sensor, selected only if its register map exposes frequency/spectral information useful for swarm detection;
- audio: analog microphone + local preamplifier, carried over a shielded audio pair to the main unit;
- main unit: STM32U535, LTE Cat-1 bis, local flash, RS-485, audio ADC path, solar/battery power management.

This avoids a second custom MCU/PCB inside the hive while keeping the audio path rich enough for FFT/MFCC/event recording.

## Representative low-volume prices

| Item | Candidate | Snapshot price | Decision |
|---|---|---:|---|
| MCU | STM32U535RET6 | EUR 3.94 | candidate |
| LTE | SIM7672E EU | EUR 18.48 | candidate |
| QSPI flash | BY25Q128ESSIG | EUR 1.02 | validate against W25Q128JV reference |
| 3V3 regulator | TPS63900 | EUR 2.36 | candidate |
| modem regulator | TPS63070 | EUR 2.89 | candidate |
| modem load switch | TPS22965 | EUR 0.75 | candidate |
| UART translator | TXU0202 | EUR 0.96 | candidate |
| RS-485 transceiver | SP3485EN-L | EUR 0.83 | candidate |
| RS-485 TVS | SM712 | EUR 0.26 | candidate |
| switched 12-V boost | TPS61088 | EUR 2.35 | candidate |
| T/RH RS-485 probe | low-cost Modbus probe | EUR 6.64 | field validation required |
| vibration RS-485 | generic 3-axis IP67 Modbus | USD 27.96 | low-cost candidate; register-map validation mandatory |
| vibration reference | WITMOTION WTVB01-485 | USD 52.60 | comparison/reference |
| analog microphone | PUI AOM-5024L-HD-F-R | EUR 3.10 | candidate |
| microphone preamp | MAX4466-class | EUR 0.87 | validate noise/current |
| 1S LiFePO4 cell | 32700 6-Ah class | EUR 2.01 advertised | supplier/capacity validation mandatory |
| 20-W solar panel | 12-V mono class | EUR 14.90 | candidate; size from winter budget |
| prototype solar charger | 1S LiFePO4 module | EUR 6.85 | prototype only |
| shielded cable | 6x0.22 mm2 | EUR 1.29/m | 5 m = EUR 6.45 |

The authoritative machine-readable list is `hardware/BOM.csv`.

## Why the 1S battery remains

The main system remains based on a **1S LiFePO4** battery. Do not move the entire electronics stack to a 12.8-V battery just because commercial RS-485 probes often accept 9-30 V.

Power tree:

```text
solar panel
    |
1S LiFePO4 charger/power path
    |
  battery ~3.2 V
    |
    +--> 3V3 low-Iq --> STM32 + flash + logic
    |
    +--> 3V8 high-current --> SIM7672E
    |
    +--> switched boost 12 V --> RS-485 probes only
```

The 12-V rail is enabled only while the external RS-485 probes need power. This avoids the conversion losses of keeping a high-voltage rail permanently active.

## Cost items intentionally not yet counted

A complete unit price is not frozen until these items have exact part numbers and assembly assumptions:

- PCB fabrication and assembly;
- DC/DC inductors and full passive networks;
- SIM/USB/input ESD arrays;
- solar/battery connector set;
- sealed 6-pin field connector;
- enclosure, glands, mounting hardware and membrane/vent;
- antenna mounting hardware;
- battery protection/BMS if not integrated in the selected cell/pack;
- wiring harness labor;
- SIM/data plan;
- certification, test fixture and production test time.

Do not publish a final unit-cost target by simply summing the headline parts above.

## Validation gates before a part becomes APPROVED

1. Purchase at least two samples from the intended supplier.
2. Record exact manufacturer, model, firmware and Modbus register map.
3. Test at 5 m cable length with LTE transmitting and solar converters active.
4. Verify cold-start voltage/current and recovery after brownout.
5. Compare vibration output against the LIS2DW12 R&D reference on the same hive.
6. Verify the T/RH probe survives condensation/propolis exposure or provide a replaceable protective arrangement.
7. Measure microphone SNR with the LTE modem attaching/transmitting.
8. Re-price at 10/100/1000 units before production freeze.
