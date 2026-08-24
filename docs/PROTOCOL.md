# Telemetry protocol

## MQTT

Use MQTT over TLS. Credentials must be provisioned per device; never compile shared production secrets into firmware.

Topic convention:

```text
sentry-bee/v1/<device-id>/telemetry
sentry-bee/v1/<device-id>/event
sentry-bee/v1/<device-id>/status
sentry-bee/v1/<device-id>/cmd
```

Recommended QoS:

- routine telemetry: QoS 1;
- alarms/events: QoS 1;
- commands: QoS 1;
- retained status only if backend semantics explicitly need it.

## Telemetry JSON v1

```json
{
  "v": 1,
  "hive": "hive-001",
  "ts": 1787581800,
  "state": "normal",
  "score": 0.18,
  "temp_c": 35.2,
  "rh": 58.4,
  "temp_slope": 0.02,
  "vib_rms": 0.014,
  "vib_b1": 0.004,
  "vib_b2": 0.002,
  "vib_b3": 0.001,
  "audio_rms": 0.017,
  "audio_zcr": 0.08,
  "audio_b1": 0.005,
  "audio_b2": 0.003,
  "audio_b3": 0.001,
  "battery_v": 4.02,
  "rssi_dbm": -79
}
```

## Event record

An event must include:

- event UUID/monotonic ID;
- start/end timestamp;
- trigger feature(s);
- pre/post feature timeline;
- optional raw vibration blob;
- optional PCM audio clip;
- firmware version and hardware revision;
- explicit `label` field initially `unknown`.

The backend/UI must allow an apiarist to later label an event, e.g. `swarm`, `queenless_confirmed`, `robbing`, `hornet_attack`, `inspection`, `transport`, `weather`, `false_positive`. This creates training data rather than hiding uncertainty.

## Offline queue

Every publishable record is first committed to local flash, then published. It is deleted/advanced only after broker acknowledgement. Network loss therefore does not lose telemetry.
