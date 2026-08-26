# Firmware architecture

## Operating principle

The product separates hive-side sensing from communication and high-level processing. Temperature/RH and vibration arrive through Modbus RTU over RS-485. Audio arrives as an analog signal and is sampled by the STM32 ADC/DMA. The LTE modem is normally off because it dominates the communication power budget.

```text
RS485 T/RH -----------\
                       \
RS485 vibration --------> acquisition -> feature normalization -> baseline/anomaly -> local queue
                         /                                           |                |
analog audio -> ADC/DMA-/                                           alarm            scheduled
                                                                    |                |
                                                                    +------ LTE -----+
```

The existing direct SHT40/LIS2DW12/T5838 drivers remain as R&D/reference paths and test fixtures; they are not the preferred product-v2 field wiring.

## Sensor polling and acquisition

Initial engineering defaults must remain configurable because the exact commercial RS-485 probes are not yet approved.

Recommended starting policy:

- T/RH Modbus probe: poll every 60 s;
- vibration probe: poll at the fastest useful cadence supported by its documented register map without losing internal spectral information;
- audio: sample analysis windows at a rate sufficient for the bee acoustic bands under study; start at 16 kHz ADC sampling;
- summary record: every 60 s;
- routine LTE upload: every 15 min;
- alarm upload: immediately if coverage and battery policy allow it.

If a vibration probe requires continuous power to compute internal frequency/spectral metrics, do not power-cycle it between polls. That behavior is a hardware-selection parameter, not something firmware should assume away.

## Modbus abstraction

The application layer must not depend directly on vendor-specific register numbers.

Introduce a normalized sensor interface that maps each approved probe to common fields such as:

```text
env.temperature_c
env.humidity_rh
vibration.rms
vibration.peak
vibration.dominant_frequency_hz
vibration.band_1
vibration.band_2
vibration.band_3
```

Each sensor adapter owns:

- slave address;
- register map;
- scaling/endian rules;
- warm-up time;
- validity/status flags;
- retry policy;
- firmware/model identification where available.

Unknown/missing values must remain explicitly invalid rather than being silently replaced by zero.

## Feature pipeline

Vibration features depend on the information exposed by the selected commercial probe.

Preferred normalized feature set:

- RMS;
- peak;
- crest factor when available;
- dominant frequency;
- energy/amplitude in biologically relevant bands when the probe exposes spectral data;
- sensor health/overrange flags.

A commercial sensor reporting only a single velocity RMS is not considered sufficient for the current research objective.

Audio features:

- RMS;
- zero-crossing rate;
- energy 80-300 Hz;
- energy 300-1200 Hz;
- energy 1200-4000 Hz;
- later MFCC/other compact descriptors if field data justifies them.

Environmental features:

- temperature;
- RH;
- one-minute temperature slope;
- one-minute RH slope.

The portable implementation may retain deterministic host-side DFT/reference code. Target firmware should use CMSIS-DSP real FFT where appropriate.

## Adaptive baseline

Each hive is its own reference. After warm-up, feature mean/variance or equivalent robust statistics are updated over time. Alarm samples must not immediately train the baseline, otherwise the device can learn an abnormal event as normal.

This remains an anomaly detector first, not a universal biological classifier. Once labelled field data exists, the normalized feature vector can feed:

1. a compact server-trained classifier exported as constants;
2. TinyML on STM32 if justified;
3. seasonal/time-of-day conditional baselines.

## Event capture

External flash should maintain a circular pre-event buffer for data that is cheap enough to retain locally.

On warning/alarm:

- retain pre-trigger audio samples/features;
- continue audio capture for a configurable post-event period;
- retain the RS-485 sensor readings and diagnostic/status registers around the event;
- retain richer vibration information when the selected probe exposes it;
- tag the event with firmware, sensor model and configuration versions;
- upload only when LTE/battery policy permits.

This is required to build a traceable labelled dataset.

## Sensor validation mode

During development, support simultaneous comparison between commercial probes and R&D reference sensors.

For vibration, the validation dataset should include both:

```text
commercial RS-485 vibration output
+
LIS2DW12 reference features/raw windows
```

with both sensors mechanically mounted as consistently as possible. This allows the low-cost probe to be accepted or rejected from measured hive data instead of datasheet claims alone.

## State machine

```text
BOOT -> SELF_TEST -> SENSOR_DISCOVERY -> BASELINE_WARMUP -> NORMAL
                                                   NORMAL -> WARNING -> ALARM
                                                      ^        |          |
                                                      +--------+----------+

NORMAL/WARNING/ALARM -> UPLOAD_DUE -> LTE_CONNECT -> PUBLISH -> LTE_OFF
                                      | failure
                                      v
                                  STORE_RETRY
```

`SENSOR_DISCOVERY` should verify expected Modbus slave identities/register availability and audio-path health. A missing probe is a device fault, not a biological hive alarm.

Recommended persistence policy before declaring a biological alarm: require multiple abnormal windows or corroboration from at least two sensor families. A one-window spike should normally trigger event capture/warning rather than a definitive conclusion.
