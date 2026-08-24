# Prototype bring-up

## Stage 0 — bench

1. Assemble STM32U535 dev/prototype board, SHT40, LIS2DW12, T5838 and SIM7672E breakout/reference board.
2. Prove each rail independently before fitting modem.
3. Verify SWD and low-power wakeup.
4. Read SHT40 and validate CRC.
5. Read LIS2DW12 WHO_AM_I and stream raw axes.
6. Capture PDM microphone through DMA and export a short PCM sample.
7. Power-cycle SIM7672E under MCU control.
8. Verify `AT`, SIM ready, LTE registration and RSSI.
9. Measure peak modem current and rail droop with an oscilloscope.

## Stage 1 — one instrumented hive

Log raw data aggressively to establish signal quality:

- exact sensor mounting position and orientation;
- vibration at 800 Hz;
- audio at 16 kHz during periodic windows;
- T/RH every minute;
- UTC time;
- weather if available server-side;
- manual inspection/event notes.

Do not tune thresholds after looking at only one interesting event.

## Stage 2 — paired accelerometer experiment

On one or two R&D hives, mount a wider-band reference accelerometer (e.g. IIS3DWB) alongside LIS2DW12. Compare classification-relevant spectral information. Keep LIS2DW12 in production only if the cheaper sensor preserves the useful signal.

## Stage 3 — field power test

Measure energy, not only current:

- sensing Wh/day;
- LTE cold attach energy;
- MQTT/TLS session energy;
- good vs weak-signal upload energy;
- retries/day;
- solar production worst-week estimate.

Then size battery and panel with seasonal margin.

## Stage 4 — dataset labels

For every confirmed event capture:

- swarm preparation / actual swarm;
- queen removal/loss if safely observed;
- normal inspection;
- feeding;
- robbing;
- predator attack;
- heavy rain/wind;
- transport/movement.

The first deliverable is a trustworthy labelled dataset. A reliable classifier comes after it.
