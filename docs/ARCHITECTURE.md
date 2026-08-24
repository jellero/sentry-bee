# Firmware architecture

## Operating principle

The node separates sensing from communication. Sensors and the STM32 collect compact features continuously or periodically; the LTE modem is normally off. This is required because the modem dominates the power budget.

```text
LIS2DW12 -----\
T5838 ---------> acquisition -> feature extraction -> baseline/anomaly -> local queue
SHT40 --------/                                         |                |
                                                       alarm            scheduled
                                                        |                |
                                                        +------ LTE -----+
```

## Recommended schedule

Initial engineering defaults, intentionally configurable:

- SHT40: one measurement every 60 s.
- LIS2DW12: 800 Hz windows; start with 512 samples per analysis window. For continuous research acquisition, use DMA/FIFO and longer overlapping windows.
- T5838: 16 kHz PDM-to-PCM analysis windows. Use DMA and CMSIS-PDM/CMSIS-DSP or equivalent on target.
- summary record: every 60 s.
- routine LTE upload: every 15 min.
- alarm upload: immediately if coverage and battery policy allow it.

## Feature pipeline

Vibration v1 features:

- RMS;
- peak;
- crest factor;
- energy 10-35 Hz;
- energy 35-120 Hz;
- energy 120-350 Hz.

Audio v1 features:

- RMS;
- zero-crossing rate;
- energy 80-300 Hz;
- energy 300-1200 Hz;
- energy 1200-4000 Hz.

Environmental features:

- temperature;
- RH;
- one-minute temperature slope;
- one-minute RH slope.

The portable implementation uses a slow DFT so it is deterministic and testable on a host. The STM32U535 target should replace it with CMSIS-DSP real FFT while retaining the same feature API.

## Adaptive baseline

Each hive is its own reference. After a warm-up period, scalar feature mean and variance are updated with an exponential moving estimator. Alarm samples are excluded from baseline learning to avoid teaching an abnormal event as normal.

This is only the v1 anomaly detector. It is not a universal biological classifier. Once labelled field data exists, the feature vector can feed:

1. a logistic/gradient-boosted classifier trained server-side and quantized to constants;
2. a tiny neural network deployed through STM32Cube.AI/TFLM;
3. a seasonal/time-of-day conditional baseline.

## Event capture

QSPI should maintain a circular pre-event buffer. On warning/alarm:

- freeze N seconds/minutes before trigger;
- retain raw vibration and short PCM audio;
- continue capture for a configurable post-event duration;
- tag the clip with sensor features and node state;
- upload only when policy permits.

This is essential for building the labelled dataset needed to improve swarm/queenless detection.

## State machine

```text
BOOT -> SELF_TEST -> BASELINE_WARMUP -> NORMAL
                                   NORMAL -> WARNING -> ALARM
                                      ^        |          |
                                      +--------+----------+

NORMAL/WARNING/ALARM -> UPLOAD_DUE -> LTE_CONNECT -> PUBLISH -> LTE_OFF
                                      | failure
                                      v
                                  STORE_RETRY
```

Recommended persistence policy before declaring an alarm: require multiple abnormal windows or corroboration from at least two sensor families. A one-window spike should normally be a warning/event capture, not a biological conclusion.
