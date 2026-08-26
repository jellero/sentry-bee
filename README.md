# Sentry-Bee

Sentry-Bee is a low-cost, field-oriented hive monitoring system focused on early detection of swarming and abnormal colony behaviour.

## Product goals

- no ESP32;
- LTE 4G only as primary WAN, no 2G dependency;
- low duty-cycle LTE with local buffering;
- vibration + acoustic + temperature/humidity sensor fusion;
- simple field wiring up to approximately 5 m between hive and main unit;
- replaceable ready-made field sensors where they reduce cost/risk;
- local feature extraction and anomaly scoring;
- MQTT/TLS telemetry with store-and-forward when LTE is unavailable;
- solar + LiFePO4 field power.

## Current product direction (v2)

The preferred field architecture separates the hive-side sensing from the external LTE/solar main unit:

```text
HIVE

RS-485 T/RH probe -------\
                          +---- shielded 5 m cable ---- MAIN UNIT ---- LTE
RS-485 vibration probe --/                           STM32U535
                                                     QSPI
analog mic + preamp ------------------------------- audio ADC/DSP
                                                     LiFePO4 + solar
```

### Sensor policy

| Function | Product direction | Notes |
|---|---|---|
| Temperature/RH | ready-made RS-485/Modbus probe | low cost, replaceable, robust long cable |
| Vibration | ready-made RS-485/Modbus probe | must expose frequency/spectral information useful for hive analysis |
| Acoustic | analog microphone + local preamp | audio runs on shielded pair, not RS-485 |
| WAN | SIMCom SIM7672E | LTE Cat-1 bis; no application dependency on 2G |
| Main MCU | STM32U535RET6 | Modbus master, DSP, event logic, LTE orchestration |
| Local storage | 128-Mbit QSPI NOR | telemetry queue, event clips, logs |

See `docs/PRODUCT_ARCHITECTURE.md` for the detailed product architecture.

## R&D/reference hardware

The earlier direct-sensor design is retained in the repository for development and sensor validation:

- LIS2DW12 vibration sensor;
- SHT40 temperature/RH sensor;
- TDK T5838 PDM microphone.

These parts are useful as a known reference when comparing low-cost commercial RS-485 probes. They are no longer the preferred product wiring architecture.

## Power architecture

The main energy store remains **1S LiFePO4**. Commercial RS-485 probes are powered from a switched boosted rail rather than moving the entire system to a 12-V battery:

```text
solar panel -> LiFePO4 charger -> 1S LiFePO4
                                  |-> 3V3 -> STM32 + logic + flash
                                  |-> 3V8 -> SIM7672E
                                  +-> switched 12V -> RS-485 probes
```

The LTE modem and external probes should not remain powered unnecessarily.

## Costing

`hardware/BOM.csv` contains the machine-readable engineering BOM with a low-volume price snapshot.

`hardware/COSTING.md` documents:

- observed low-volume component prices;
- which parts are only validation candidates;
- cost items not yet included;
- validation gates before a marketplace/low-cost part can become production-approved.

The current largest sensor cost risk is the vibration probe. The plan is to compare a low-cost 3-axis RS-485 candidate against both a known commercial reference and the LIS2DW12 R&D reference on the same hive.

## Repository layout

- `firmware/` portable firmware core, sensor/register drivers and host-side tests.
- `firmware/boards/stm32u535/` first STM32U535 board/HAL integration material.
- `backend/` MQTT ingestion service, SQLite storage, REST API and event labeling.
- `deploy/` local broker configuration.
- `docs/PRODUCT_ARCHITECTURE.md` current product field architecture.
- `docs/ARCHITECTURE.md` sensing/anomaly firmware architecture.
- `docs/HARDWARE.md` hardware design rules.
- `docs/PROTOCOL.md` MQTT topics and payloads.
- `docs/BRINGUP.md` prototype bring-up sequence.
- `docs/REFERENCES.md` component and research references.
- `hardware/BOM.csv` costed engineering BOM.
- `hardware/COSTING.md` costing assumptions and validation gates.
- `hardware/PINOUT.md` rev-A direct-sensor/R&D board pinout.
- `hardware/SCHEMATIC.md` rev-A direct-sensor/R&D schematic rules.

## Firmware

The repository contains an executable host model of the feature/anomaly pipeline and hardware-facing drivers independent from STM32Cube HAL.

Build and test:

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
./build/sentry_bee_host
```

The existing SHT40/LIS2DW12 drivers remain relevant for the reference board. Product-v2 firmware additionally needs a Modbus RTU master, switched sensor-power sequencing and analog audio ADC/DMA acquisition.

## Development backend

```bash
docker compose up --build
```

Development services:

- Mosquitto broker on port `1883`;
- FastAPI backend on port `8000`;
- persistent SQLite telemetry/event storage.

Useful endpoints:

```text
GET  /health
GET  /api/hives
GET  /api/hives/{hive_id}/latest
GET  /api/hives/{hive_id}/history
GET  /api/events
POST /api/events/{event_id}/label
```

The development broker configuration is not production security. Production requires TLS, per-device credentials and ACLs.

## Detection model

The firmware does **not** claim to classify queen loss or swarming with a universal pretrained model. It builds observables and an adaptive baseline per hive:

- temperature/humidity and slopes;
- vibration frequency/amplitude features from the selected probe or R&D reference;
- audio RMS/spectral features;
- fused anomaly score;
- event persistence/severity.

Production thresholds/models must be trained from labelled field data collected across real hives, climates and seasons.

## Connectivity policy

Normal operation buffers summaries locally and powers LTE only for scheduled uploads. A high-confidence event can trigger an immediate upload. If LTE is unavailable, records remain queued in QSPI and are retried later.

## Current engineering boundary

The v2 architecture is frozen at system level, but several production parts remain in `VALIDATE`/`TBD` state. In particular:

- exact RS-485 temperature/RH probe;
- exact RS-485 vibration probe and its register map;
- microphone preamp/line topology after 5-m LTE interference test;
- sealed field connector;
- production solar charger and battery capacity;
- full PCB/assembly/enclosure cost.

Those are intentionally marked rather than guessed.

## License

MIT.
