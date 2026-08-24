#include "sentry_bee/sentry_bee.h"
#include <stdio.h>

static const char *state_name(sb_health_state_t s) {
    switch (s) {
        case SB_STATE_WARNING: return "warning";
        case SB_STATE_ALARM: return "alarm";
        default: return "normal";
    }
}

int sb_telemetry_json(char *dst, size_t dst_len, const char *hive_id,
                      const sb_snapshot_t *s) {
    if (!dst || !dst_len || !hive_id || !s) return -1;
    return snprintf(dst, dst_len,
        "{\"v\":1,\"hive\":\"%s\",\"ts\":%llu,\"state\":\"%s\","
        "\"score\":%.3f,\"temp_c\":%.2f,\"rh\":%.2f,\"temp_slope\":%.3f,"
        "\"vib_rms\":%.6f,\"vib_b1\":%.6f,\"vib_b2\":%.6f,\"vib_b3\":%.6f,"
        "\"audio_rms\":%.6f,\"audio_zcr\":%.4f,\"audio_b1\":%.6f,"
        "\"audio_b2\":%.6f,\"audio_b3\":%.6f,\"battery_v\":%.3f,\"rssi_dbm\":%d}",
        hive_id, (unsigned long long)s->timestamp, state_name(s->state), s->anomaly_score,
        s->env.temperature_c, s->env.humidity_rh, s->env.temperature_slope_c_per_min,
        s->vibration.rms, s->vibration.band_1, s->vibration.band_2, s->vibration.band_3,
        s->audio.rms, s->audio.zero_crossing_rate, s->audio.band_1,
        s->audio.band_2, s->audio.band_3, s->battery_v, s->rssi_dbm);
}
