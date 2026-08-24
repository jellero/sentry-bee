#include "sentry_bee/sentry_bee.h"
#include "sentry_bee/config.h"
#include <math.h>
#include <string.h>

static void baseline_update(sb_baseline_scalar_t *b, float x) {
    if (b->count == 0) {
        b->mean = x;
        b->variance = 0.0f;
        b->count = 1;
        return;
    }
    float delta = x - b->mean;
    float alpha = b->count < SB_MIN_BASELINE_SAMPLES ? 1.0f / (float)(b->count + 1U) : SB_BASELINE_ALPHA;
    b->mean += alpha * delta;
    b->variance = (1.0f - alpha) * (b->variance + alpha * delta * delta);
    b->count++;
}

static float z_abs(const sb_baseline_scalar_t *b, float x) {
    if (b->count < SB_MIN_BASELINE_SAMPLES) return 0.0f;
    float sigma = sqrtf(fmaxf(b->variance, 1e-8f));
    return fabsf(x - b->mean) / sigma;
}

static float squash_z(float z) {
    if (z <= 1.0f) return 0.0f;
    return 1.0f - expf(-(z - 1.0f) / 2.0f);
}

void sb_app_init(sb_app_t *app, const char *hive_id) {
    if (!app) return;
    memset(app, 0, sizeof(*app));
    if (hive_id) {
        strncpy(app->hive_id, hive_id, sizeof(app->hive_id) - 1U);
        app->hive_id[sizeof(app->hive_id) - 1U] = '\0';
    }
}

float sb_app_score_and_update(sb_app_t *app, sb_snapshot_t *s) {
    if (!app || !s) return 0.0f;

    float scores[6] = {
        squash_z(z_abs(&app->baseline.temp, s->env.temperature_c)),
        squash_z(z_abs(&app->baseline.temp_slope, s->env.temperature_slope_c_per_min)),
        squash_z(z_abs(&app->baseline.vib_rms, s->vibration.rms)),
        squash_z(z_abs(&app->baseline.vib_band1, s->vibration.band_1)),
        squash_z(z_abs(&app->baseline.audio_rms, s->audio.rms)),
        squash_z(z_abs(&app->baseline.audio_band1, s->audio.band_1))
    };

    float max1 = 0.0f, max2 = 0.0f;
    for (size_t i = 0; i < 6; ++i) {
        if (scores[i] > max1) { max2 = max1; max1 = scores[i]; }
        else if (scores[i] > max2) { max2 = scores[i]; }
    }
    s->anomaly_score = 0.70f * max1 + 0.30f * max2;
    if (s->anomaly_score >= SB_ALARM_SCORE) s->state = SB_STATE_ALARM;
    else if (s->anomaly_score >= SB_WARNING_SCORE) s->state = SB_STATE_WARNING;
    else s->state = SB_STATE_NORMAL;

    if (s->state != SB_STATE_ALARM) {
        baseline_update(&app->baseline.temp, s->env.temperature_c);
        baseline_update(&app->baseline.temp_slope, s->env.temperature_slope_c_per_min);
        baseline_update(&app->baseline.vib_rms, s->vibration.rms);
        baseline_update(&app->baseline.vib_band1, s->vibration.band_1);
        baseline_update(&app->baseline.audio_rms, s->audio.rms);
        baseline_update(&app->baseline.audio_band1, s->audio.band_1);
    }

    app->previous = *s;
    app->has_previous = true;
    return s->anomaly_score;
}
