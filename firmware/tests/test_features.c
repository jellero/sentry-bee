#include "sentry_bee/sentry_bee.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    float x[800];
    const float pi2 = 6.2831853071795864769f;
    for (size_t i = 0; i < 800; ++i) x[i] = sinf(pi2 * 20.0f * (float)i / 800.0f);

    float rms = sb_compute_rms(x, 800);
    assert(fabsf(rms - 0.7071f) < 0.01f);
    float e20 = sb_compute_band_energy(x, 800, 800.0f, 10.0f, 35.0f);
    float e90 = sb_compute_band_energy(x, 800, 800.0f, 70.0f, 110.0f);
    assert(e20 > e90 * 50.0f);

    sb_app_t app;
    sb_app_init(&app, "test");
    sb_snapshot_t s;
    memset(&s, 0, sizeof(s));
    for (int i = 0; i < 40; ++i) {
        s.env.temperature_c = 35.0f + 0.01f * (float)(i % 3);
        s.env.temperature_slope_c_per_min = 0.01f;
        s.vibration.rms = 0.02f + 0.0001f * (float)(i % 2);
        s.vibration.band_1 = 0.001f + 0.00001f * (float)(i % 2);
        s.audio.rms = 0.03f + 0.0001f * (float)(i % 2);
        s.audio.band_1 = 0.002f + 0.00001f * (float)(i % 2);
        sb_app_score_and_update(&app, &s);
    }
    s.vibration.rms = 0.20f;
    s.audio.rms = 0.30f;
    sb_app_score_and_update(&app, &s);
    assert(s.anomaly_score > 0.5f);

    puts("ok");
    return 0;
}
