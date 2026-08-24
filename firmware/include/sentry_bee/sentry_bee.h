#ifndef SENTRY_BEE_H
#define SENTRY_BEE_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "sentry_bee/platform.h"

#define SB_HIVE_ID_MAX 24

typedef enum {
    SB_STATE_NORMAL = 0,
    SB_STATE_WARNING,
    SB_STATE_ALARM
} sb_health_state_t;

typedef struct {
    float temperature_c;
    float humidity_rh;
    float temperature_slope_c_per_min;
    float humidity_slope_per_min;
} sb_env_features_t;

typedef struct {
    float rms;
    float peak;
    float crest_factor;
    float band_1;
    float band_2;
    float band_3;
} sb_vibration_features_t;

typedef struct {
    float rms;
    float zero_crossing_rate;
    float band_1;
    float band_2;
    float band_3;
} sb_audio_features_t;

typedef struct {
    uint64_t timestamp;
    sb_env_features_t env;
    sb_vibration_features_t vibration;
    sb_audio_features_t audio;
    float anomaly_score;
    sb_health_state_t state;
    float battery_v;
    int16_t rssi_dbm;
} sb_snapshot_t;

typedef struct {
    float mean;
    float variance;
    uint32_t count;
} sb_baseline_scalar_t;

typedef struct {
    sb_baseline_scalar_t temp;
    sb_baseline_scalar_t temp_slope;
    sb_baseline_scalar_t vib_rms;
    sb_baseline_scalar_t vib_band1;
    sb_baseline_scalar_t audio_rms;
    sb_baseline_scalar_t audio_band1;
} sb_baseline_t;

typedef struct {
    char hive_id[SB_HIVE_ID_MAX];
    sb_baseline_t baseline;
    sb_snapshot_t previous;
    bool has_previous;
} sb_app_t;

void sb_app_init(sb_app_t *app, const char *hive_id);
float sb_compute_rms(const float *samples, size_t n);
float sb_compute_peak(const float *samples, size_t n);
float sb_compute_zero_crossing_rate(const float *samples, size_t n);
float sb_compute_band_energy(const float *samples, size_t n, float sample_hz,
                             float min_hz, float max_hz);
void sb_extract_vibration_features(const float *samples, size_t n, float sample_hz,
                                   sb_vibration_features_t *out);
void sb_extract_audio_features(const float *samples, size_t n, float sample_hz,
                               sb_audio_features_t *out);
float sb_app_score_and_update(sb_app_t *app, sb_snapshot_t *snapshot);
int sb_telemetry_json(char *dst, size_t dst_len, const char *hive_id,
                      const sb_snapshot_t *s);

bool sb_sht40_read(const sb_platform_t *p, float *temperature_c, float *humidity_rh);
bool sb_lis2dw12_init_800hz(const sb_platform_t *p);
bool sb_lis2dw12_read_xyz(const sb_platform_t *p, int16_t xyz[3]);
bool sb_sim7672_basic_check(const sb_platform_t *p);
bool sb_sim7672_wait_network(const sb_platform_t *p, uint32_t timeout_ms);
bool sb_sim7672_get_rssi(const sb_platform_t *p, int16_t *rssi_dbm);

#endif
