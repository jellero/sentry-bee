# Sentry-Bee

Sentry-Bee is a low-cost, field-oriented hive monitoring node focused on early detection of swarming and abnormal colony behaviour.

## v1 design goals

- no ESP32;
- LTE 4G only as primary WAN, no 2G dependency;
- always-on sensing with low duty-cycle radio;
- vibration + acoustic + temperature/humidity sensor fusion;
- local feature extraction and anomaly scoring;
- raw data retained only around interesting events;
- MQTT/TLS telemetry with store-and-forward when LTE is unavailable;
- hardware suitable for a custom STM32U535 board.

## Reference hardware

| Function | Part | Notes |
|---|---|---|
| MCU | STM32U535 | Cortex-M33, DSP/FPU, low-power |
| LTE | SIMCom SIM7672E | LTE Cat-1 bis, LTE-only SKU for EU |
| Vibration | ST LIS2DW12 | 3-axis, up to 1.6 kHz ODR |
| Acoustic | TDK T5838 | PDM MEMS microphone |
| Temp/RH | Sensirion SHT40 | calibrated digital T/RH |
| Local storage | 16-32 MB QSPI NOR | event clips, logs, offline queue |

The LIS2DW12 should be on a small mechanically coupled daughterboard fixed to the hive body/frame support. The microphone and SHT40 need protected airflow/acoustic access and must not be buried inside the main sealed electronics enclosure.

## Repository layout

- `firmware/` portable firmware core, drivers and host-side tests.
- `docs/ARCHITECTURE.md` state machine, sampling and anomaly strategy.
- `docs/HARDWARE.md` electrical/mechanical design rules and power budget.
- `docs/PROTOCOL.md` MQTT topics and payloads.
- `docs/BRINGUP.md` prototype bring-up sequence.
- `hardware/BOM.csv` initial engineering BOM.

## Firmware status

The repository contains an executable host model of the v1 pipeline plus hardware-facing drivers that are independent from STM32Cube HAL. The board-specific layer must implement the callbacks in `platform.h` and connect them to STM32U535 HAL/LL, DMA, RTC, QSPI and UART peripherals.

Build and test on Linux/macOS:

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
./build/sentry_bee_host
```

## Detection model in v1

The first firmware does **not** claim to classify queen loss or swarming with a pretrained universal model. It computes robust observable features and an adaptive baseline per hive:

- temperature and humidity level/slope;
- vibration RMS, peak, crest factor and energy in configurable bands;
- audio RMS, zero-crossing rate and band energies;
- fused anomaly score;
- event persistence and severity.

This is intentional: production thresholds/models must be trained from labelled data collected on real hives, climates and seasons.

## Connectivity policy

Normal operation buffers summaries locally and powers LTE only for scheduled uploads. A high-confidence event can trigger an immediate upload. If LTE is unavailable, records remain queued in QSPI and are retried later. No 2G fallback is required by the application.

## License

MIT.
