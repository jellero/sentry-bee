#ifndef SENTRY_BEE_CONFIG_H
#define SENTRY_BEE_CONFIG_H

/* Sampling defaults. Tune from real hive data, not intuition. */
#define SB_VIB_SAMPLE_HZ              800U
#define SB_AUDIO_SAMPLE_HZ          16000U
#define SB_VIB_WINDOW_SAMPLES        512U
#define SB_AUDIO_WINDOW_SAMPLES     1024U
#define SB_ENV_PERIOD_SECONDS         60U
#define SB_FEATURE_PERIOD_SECONDS     60U
#define SB_UPLOAD_PERIOD_SECONDS     900U
#define SB_EVENT_UPLOAD_SCORE        0.80f
#define SB_WARNING_SCORE             0.60f
#define SB_ALARM_SCORE               0.80f
#define SB_BASELINE_ALPHA            0.02f
#define SB_MIN_BASELINE_SAMPLES        30U

/* Vibration bands, chosen to preserve low-frequency swarm-related content. */
#define SB_VIB_BAND1_MIN_HZ           10.0f
#define SB_VIB_BAND1_MAX_HZ           35.0f
#define SB_VIB_BAND2_MIN_HZ           35.0f
#define SB_VIB_BAND2_MAX_HZ          120.0f
#define SB_VIB_BAND3_MIN_HZ          120.0f
#define SB_VIB_BAND3_MAX_HZ          350.0f

/* Audio bands are deliberately broad in v1. */
#define SB_AUDIO_BAND1_MIN_HZ         80.0f
#define SB_AUDIO_BAND1_MAX_HZ        300.0f
#define SB_AUDIO_BAND2_MIN_HZ        300.0f
#define SB_AUDIO_BAND2_MAX_HZ       1200.0f
#define SB_AUDIO_BAND3_MIN_HZ       1200.0f
#define SB_AUDIO_BAND3_MAX_HZ       4000.0f

#endif
