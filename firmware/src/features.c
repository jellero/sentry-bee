#include "sentry_bee/sentry_bee.h"
#include "sentry_bee/config.h"
#include <math.h>

float sb_compute_rms(const float *samples, size_t n) {
    if (!samples || n == 0) return 0.0f;
    double sum = 0.0;
    for (size_t i = 0; i < n; ++i) sum += (double)samples[i] * samples[i];
    return (float)sqrt(sum / (double)n);
}

float sb_compute_peak(const float *samples, size_t n) {
    float peak = 0.0f;
    if (!samples) return peak;
    for (size_t i = 0; i < n; ++i) {
        float v = fabsf(samples[i]);
        if (v > peak) peak = v;
    }
    return peak;
}

float sb_compute_zero_crossing_rate(const float *samples, size_t n) {
    if (!samples || n < 2) return 0.0f;
    size_t crossings = 0;
    for (size_t i = 1; i < n; ++i) {
        if ((samples[i - 1] < 0.0f && samples[i] >= 0.0f) ||
            (samples[i - 1] >= 0.0f && samples[i] < 0.0f)) crossings++;
    }
    return (float)crossings / (float)(n - 1);
}

/* Portable O(N*K) DFT estimator. Replace with CMSIS-DSP RFFT on STM32. */
float sb_compute_band_energy(const float *samples, size_t n, float sample_hz,
                             float min_hz, float max_hz) {
    if (!samples || n < 4 || sample_hz <= 0.0f || min_hz >= max_hz) return 0.0f;
    size_t k_min = (size_t)ceilf(min_hz * (float)n / sample_hz);
    size_t k_max = (size_t)floorf(max_hz * (float)n / sample_hz);
    size_t nyquist = n / 2;
    if (k_max > nyquist) k_max = nyquist;
    if (k_min > k_max) return 0.0f;

    double energy = 0.0;
    const double pi2 = 6.2831853071795864769;
    for (size_t k = k_min; k <= k_max; ++k) {
        double re = 0.0, im = 0.0;
        for (size_t i = 0; i < n; ++i) {
            double w = 0.5 - 0.5 * cos(pi2 * (double)i / (double)(n - 1));
            double phase = pi2 * (double)k * (double)i / (double)n;
            re += (double)samples[i] * w * cos(phase);
            im -= (double)samples[i] * w * sin(phase);
        }
        energy += re * re + im * im;
    }
    return (float)(energy / ((double)n * (double)n));
}

void sb_extract_vibration_features(const float *samples, size_t n, float sample_hz,
                                   sb_vibration_features_t *out) {
    if (!out) return;
    out->rms = sb_compute_rms(samples, n);
    out->peak = sb_compute_peak(samples, n);
    out->crest_factor = out->rms > 1e-9f ? out->peak / out->rms : 0.0f;
    out->band_1 = sb_compute_band_energy(samples, n, sample_hz, SB_VIB_BAND1_MIN_HZ, SB_VIB_BAND1_MAX_HZ);
    out->band_2 = sb_compute_band_energy(samples, n, sample_hz, SB_VIB_BAND2_MIN_HZ, SB_VIB_BAND2_MAX_HZ);
    out->band_3 = sb_compute_band_energy(samples, n, sample_hz, SB_VIB_BAND3_MIN_HZ, SB_VIB_BAND3_MAX_HZ);
}

void sb_extract_audio_features(const float *samples, size_t n, float sample_hz,
                               sb_audio_features_t *out) {
    if (!out) return;
    out->rms = sb_compute_rms(samples, n);
    out->zero_crossing_rate = sb_compute_zero_crossing_rate(samples, n);
    out->band_1 = sb_compute_band_energy(samples, n, sample_hz, SB_AUDIO_BAND1_MIN_HZ, SB_AUDIO_BAND1_MAX_HZ);
    out->band_2 = sb_compute_band_energy(samples, n, sample_hz, SB_AUDIO_BAND2_MIN_HZ, SB_AUDIO_BAND2_MAX_HZ);
    out->band_3 = sb_compute_band_energy(samples, n, sample_hz, SB_AUDIO_BAND3_MIN_HZ, SB_AUDIO_BAND3_MAX_HZ);
}
