# Product architecture v2

This is the current product direction for Sentry-Bee. The earlier direct-sensor STM32 board remains useful as an R&D/reference platform, but it is not the preferred field-product architecture.

## Physical split

```text
HIVE / SENSOR SIDE

  RS-485 temperature/RH probe ----\
                                    \
  RS-485 vibration probe -----------+---- shielded 5 m multicore cable ---- MAIN UNIT
                                    /
  analog microphone + preamp ------/

MAIN UNIT

  STM32U535
  RS-485 transceiver
  audio ADC / DSP
  local QSPI flash
  SIM7672E LTE Cat-1 bis
  LiFePO4 battery + solar charger
  switched 12-V rail for probes
```

The target cable length is **5 m**. A design that is robust at 5 m naturally covers the 2-3 m installations.

## Why ready-made RS-485 sensors

Temperature/RH and vibration are slow/control-oriented measurements and are well suited to rugged commercial Modbus probes. Buying them ready-made removes several product risks:

- long-distance I2C/SPI wiring;
- a second custom PCB and MCU in the hive;
- sensor-head enclosure and field-service complexity;
- EMC debugging for raw digital sensor buses;
- separate sensor-head firmware/bootloader/update flow.

The sensor itself is replaceable without replacing the LTE/solar main unit.

## Vibration sensor acceptance requirement

A vibration probe is **not acceptable** just because it reports a Modbus vibration value.

It must expose enough information for biological-event analysis. Preferred data, in descending order:

1. raw or near-raw acceleration windows;
2. spectral bins / dominant frequencies with amplitudes;
3. configurable band energies;
4. at minimum dominant vibration frequency plus RMS/peak/crest-type metrics.

A sensor that reports only one RMS velocity number is not sufficient for the swarm-detection research target.

The low-cost generic RS-485 vibration sensor and WITMOTION WTVB01-485 are therefore validation candidates, not automatically production-approved parts.

## Audio path

Audio does **not** use RS-485.

The microphone is analog and the preamplifier sits physically close to the capsule. The preamp output is low impedance and travels to the main unit over a shielded pair.

Preferred first prototype:

```text
microphone -> low-noise preamp -> shielded pair -> anti-alias filter -> STM32 ADC/DMA
```

If LTE/switching interference is excessive at 5 m, upgrade the cable path to balanced audio:

```text
microphone -> preamp -> differential line driver
                         ||
                  shielded twisted pair
                         ||
              differential receiver -> ADC
```

Do not carry a raw electret/MEMS capsule signal five metres before amplification.

## Field cable

Target: one shielded multicore cable, approximately 5 m.

Initial allocation using three twisted pairs:

| Pair | Signal |
|---|---|
| 1 | RS485-A / RS485-B |
| 2 | +12V_SENSOR / GND |
| 3 | AUDIO / AUDIO_RETURN or AUDIO+ / AUDIO- |
| shield | chassis/EMC termination at main unit; configurable termination strategy |

Use a sealed connector. Final choice between M8/M12 and a sealed automotive connector is cost/assembly driven.

### RS-485 rules

- Modbus RTU half duplex;
- termination only at bus ends;
- TVS at the main-unit cable entry;
- bias network located deliberately, not duplicated blindly inside every sensor;
- configurable baud rate; start conservatively (e.g. 115200 or below) before increasing it;
- unique slave address per probe;
- sensor power can be switched independently by the main unit.

## Power architecture

Keep the main energy store at 1S LiFePO4.

```text
solar panel
   |
solar / LiFePO4 charger
   |
1S LiFePO4
   |
   +--> 3.3 V low-Iq rail --> STM32 + flash + logic
   |
   +--> 3.8 V modem rail --> SIM7672E
   |
   +--> switched 12 V boost --> external RS-485 probes
```

The 12-V probe rail is not intended to be permanently enabled. The firmware should power the probes, allow warm-up, poll them, then disable the rail when the selected sensor models permit this duty cycle.

If a selected vibration sensor requires continuous operation to maintain an internal spectral estimator, that requirement must be measured and incorporated into the solar/battery budget before production freeze.

## Main-unit responsibilities

- RTC/timekeeping;
- Modbus master and sensor health monitoring;
- ADC/DMA audio acquisition;
- local FFT/features and event scoring;
- local queue/event clips in external flash;
- LTE attach, MQTT/TLS and store-and-forward;
- battery/solar telemetry;
- watchdog and hard modem recovery;
- remote configuration and eventually OTA.

## R&D reference hardware

The existing LIS2DW12, SHT40 and T5838 code is retained intentionally. It provides a known reference system for validating ready-made sensors.

For vibration in particular, mount the LIS2DW12 reference and the commercial RS-485 probe on the same hive structure and compare:

- dominant frequency;
- RMS/peak behavior;
- event timing;
- repeatability;
- sensitivity to mounting position;
- response during confirmed swarming/pre-swarming events.

Only after this comparison should the commercial vibration sensor become the sole production sensor.
