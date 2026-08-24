#include "sentry_bee/sentry_bee.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    sb_app_t app;
    sb_app_init(&app, "hive-001");

    float vib[512];
    float audio[1024];
    const float pi2 = 6.2831853071795864769f;

    for (int minute = 0; minute < 40; ++minute) {
        float event_gain = minute >= 35 ? 6.0f : 1.0f;
        for (size_t i = 0; i < 512; ++i)
            vib[i] = event_gain * 0.02f * sinf(pi2 * 20.0f * (float)i / 800.0f)
                   + 0.003f * sinf(pi2 * 90.0f * (float)i / 800.0f);
        for (size_t i = 0; i < 1024; ++i)
            audio[i] = event_gain * 0.01f * sinf(pi2 * 220.0f * (float)i / 16000.0f)
                     + 0.004f * sinf(pi2 * 850.0f * (float)i / 16000.0f);

        sb_snapshot_t s;
        memset(&s, 0, sizeof(s));
        s.timestamp = 1787580000ULL + (uint64_t)minute * 60ULL;
        s.env.temperature_c = 35.0f + (minute >= 35 ? 1.8f : 0.02f * sinf((float)minute));
        s.env.humidity_rh = 58.0f;
        s.env.temperature_slope_c_per_min = minute >= 35 ? 0.45f : 0.01f;
        sb_extract_vibration_features(vib, 512, 800.0f, &s.vibration);
        sb_extract_audio_features(audio, 1024, 16000.0f, &s.audio);
        s.battery_v = 4.05f;
        s.rssi_dbm = -77;
        sb_app_score_and_update(&app, &s);

        char json[768];
        sb_telemetry_json(json, sizeof(json), app.hive_id, &s);
        if (minute == 0 || minute >= 34) puts(json);
    }
    return 0;
}
