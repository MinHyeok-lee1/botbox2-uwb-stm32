#include "uwb_filter.h"
#include <stdio.h>
#include <math.h>

#ifndef UWB_VERBOSE_TAG_DIAG_LOG_ENABLE
#define UWB_VERBOSE_TAG_DIAG_LOG_ENABLE 0
#endif

#ifndef TAG_VERBOSE_LOGS
#define TAG_VERBOSE_LOGS UWB_VERBOSE_TAG_DIAG_LOG_ENABLE
#endif

#ifndef UWB_KF_DEBUG_LOGS
#define UWB_KF_DEBUG_LOGS 0
#endif

#ifndef UWB_CH9_FILTER_DEBUG
#define UWB_CH9_FILTER_DEBUG 0
#endif

typedef struct {
    uint32_t n;
    uint64_t sum_raw;
    uint64_t sum_med;
    uint64_t sum_iir;
    uint64_t sumsq_raw;
    uint64_t sumsq_med;
    uint64_t sumsq_iir;
    int64_t  sumerr_raw;
    int64_t  sumerr_med;
    int64_t  sumerr_iir;
} twr_gt_stats_t;

static twr_gt_stats_t g_twr_gt = {0};

#if defined(TAG_VERBOSE_LOGS) && TAG_VERBOSE_LOGS
static uint32_t twr_isqrt_u64(uint64_t x)
{
    uint64_t op = x;
    uint64_t res = 0U;
    uint64_t one = (uint64_t)1U << 62;

    while (one > op) {
        one >>= 2;
    }

    while (one != 0U) {
        if (op >= (res + one)) {
            op -= (res + one);
            res = (res >> 1) + one;
        } else {
            res >>= 1;
        }
        one >>= 2;
    }
    return (uint32_t)res;
}
#endif

static void twr_gt_stats_update(uint32_t raw_mm, uint32_t med_mm, uint32_t iir_mm)
{
#if (TWR_GT_ENABLE == 1U)
    const uint32_t gt = TWR_GT_TARGET_MM;

    g_twr_gt.n++;

    g_twr_gt.sum_raw += (uint64_t)raw_mm;
    g_twr_gt.sum_med += (uint64_t)med_mm;
    g_twr_gt.sum_iir += (uint64_t)iir_mm;

    g_twr_gt.sumsq_raw += (uint64_t)raw_mm * (uint64_t)raw_mm;
    g_twr_gt.sumsq_med += (uint64_t)med_mm * (uint64_t)med_mm;
    g_twr_gt.sumsq_iir += (uint64_t)iir_mm * (uint64_t)iir_mm;

    g_twr_gt.sumerr_raw += (int64_t)((int32_t)raw_mm - (int32_t)gt);
    g_twr_gt.sumerr_med += (int64_t)((int32_t)med_mm - (int32_t)gt);
    g_twr_gt.sumerr_iir += (int64_t)((int32_t)iir_mm - (int32_t)gt);

    if ((TWR_GT_PRINT_EVERY != 0U) && ((g_twr_gt.n % TWR_GT_PRINT_EVERY) == 0U)) {
#if defined(TAG_VERBOSE_LOGS) && TAG_VERBOSE_LOGS
        const uint32_t n = g_twr_gt.n;
#endif

#if defined(TAG_VERBOSE_LOGS) && TAG_VERBOSE_LOGS
        const uint32_t mean_raw = (uint32_t)(g_twr_gt.sum_raw / (uint64_t)n);
        const uint32_t mean_med = (uint32_t)(g_twr_gt.sum_med / (uint64_t)n);
        const uint32_t mean_iir = (uint32_t)(g_twr_gt.sum_iir / (uint64_t)n);

        const uint64_t ex2_raw = (g_twr_gt.sumsq_raw / (uint64_t)n);
        const uint64_t ex2_med = (g_twr_gt.sumsq_med / (uint64_t)n);
        const uint64_t ex2_iir = (g_twr_gt.sumsq_iir / (uint64_t)n);

        const uint64_t var_raw = (ex2_raw > ((uint64_t)mean_raw * (uint64_t)mean_raw)) ? (ex2_raw - ((uint64_t)mean_raw * (uint64_t)mean_raw)) : 0U;
        const uint64_t var_med = (ex2_med > ((uint64_t)mean_med * (uint64_t)mean_med)) ? (ex2_med - ((uint64_t)mean_med * (uint64_t)mean_med)) : 0U;
        const uint64_t var_iir = (ex2_iir > ((uint64_t)mean_iir * (uint64_t)mean_iir)) ? (ex2_iir - ((uint64_t)mean_iir * (uint64_t)mean_iir)) : 0U;

        const uint32_t std_raw = twr_isqrt_u64(var_raw);
        const uint32_t std_med = twr_isqrt_u64(var_med);
        const uint32_t std_iir = twr_isqrt_u64(var_iir);

        const int32_t err_mean_raw = (int32_t)(g_twr_gt.sumerr_raw / (int64_t)n);
        const int32_t err_mean_med = (int32_t)(g_twr_gt.sumerr_med / (int64_t)n);
        const int32_t err_mean_iir = (int32_t)(g_twr_gt.sumerr_iir / (int64_t)n);

        printf("[TWR][GT] N=%lu GT=%lu mm | raw mean=%lu std=%lu err_mean=%ld | med mean=%lu std=%lu err_mean=%ld | iir mean=%lu std=%lu err_mean=%ld\n",
               (unsigned long)n, (unsigned long)gt,
               (unsigned long)mean_raw, (unsigned long)std_raw, (long)err_mean_raw,
               (unsigned long)mean_med, (unsigned long)std_med, (long)err_mean_med,
               (unsigned long)mean_iir, (unsigned long)std_iir, (long)err_mean_iir);
#endif
    }
#else
    (void)raw_mm; (void)med_mm; (void)iir_mm;
#endif
}

void twr_reason_add(char* buf, size_t buf_sz, const char* token)
{
    if ((buf == NULL) || (buf_sz == 0U) || (token == NULL) || (token[0] == '\0')) {
        return;
    }

    size_t len = 0U;
    while ((len < buf_sz) && (buf[len] != '\0')) {
        len++;
    }
    if (len >= (buf_sz - 1U)) {
        return;
    }

    if (len != 0U) {
        buf[len++] = '+';
        if (len >= (buf_sz - 1U)) {
            buf[buf_sz - 1U] = '\0';
            return;
        }
        buf[len] = '\0';
    }

    for (size_t i = 0U; (token[i] != '\0') && (len < (buf_sz - 1U)); i++) {
        buf[len++] = token[i];
    }
    buf[len] = '\0';
}

void twr_dist_filter_reset(twr_dist_filter_t *f)
{
    memset(f, 0, sizeof(*f));
}

void uwb_distance_lpf_reset(uwb_distance_lpf_t *f)
{
    if (f == NULL) {
        return;
    }
    memset(f, 0, sizeof(*f));
    f->base_alpha_permille = UWB_DISTANCE_LPF_BASE_ALPHA_PERMILLE;

#if defined(UWB_RF_CHANNEL) && (UWB_RF_CHANNEL == 9) && UWB_CH9_FILTER_DEBUG
    static uint8_t profile_logged = 0U;
    if (profile_logged == 0U) {
        profile_logged = 1U;
        printf("[UWB][FILTER][CH9][5HZ_MP_TOL] rate=5Hz slot=55 guard=6500 max_step=%ld spike=%ld jump=%ld stable=%ld resync=%ld alpha_base=%ld alpha_min=%ld alpha_max=%ld kf_q_dist=%ld kf_q_vel=%ld kf_r=%ld nlos_scale=%ld gate=%ld kf_damp_x100=%ld kf_max_vel=%ld\r\n",
               (long)UWB_ANC_LPF_MAX_STEP_MM,
               (long)UWB_ANC_LPF_SPIKE_THRESHOLD_MM,
               (long)UWB_ANC_LPF_JUMP_THRESHOLD_MM,
               (long)UWB_ANC_LPF_STABLE_SPREAD_MM,
               (long)UWB_ANC_LPF_RESYNC_DELTA_MM,
               (long)UWB_DISTANCE_LPF_BASE_ALPHA_PERMILLE,
               (long)UWB_DISTANCE_LPF_MIN_ALPHA_PERMILLE,
               (long)UWB_DISTANCE_LPF_MAX_ALPHA_PERMILLE,
               (long)UWB_KF_Q_DIST_MM2,
               (long)UWB_KF_Q_VEL_MM2S2,
               (long)UWB_KF_R_BASE_MM2,
               (long)UWB_KF_R_NLOS_SCALE,
               (long)UWB_KF_GATE_THRESHOLD,
               (long)(UWB_KF_VEL_DAMPING * 100.0f),
               (long)UWB_KF_MAX_VEL_MM_S);
    }
#endif
}

static int32_t uwb_abs_i32(int32_t x)
{
    return (x < 0) ? -x : x;
}

static bool uwb_distance_lpf_is_spike(const uwb_distance_lpf_t *f)
{
    if ((f == NULL) || (f->count < 3U)) {
        return false;
    }

    int32_t eff_spike_thresh = (f->spike_threshold_mm > 0L)
        ? f->spike_threshold_mm : UWB_DISTANCE_LPF_SPIKE_THRESHOLD_MM;

    uint8_t i1 = (uint8_t)((f->idx + UWB_DISTANCE_LPF_RECENT_SIZE - 1U) % UWB_DISTANCE_LPF_RECENT_SIZE);
    uint8_t i2 = (uint8_t)((f->idx + UWB_DISTANCE_LPF_RECENT_SIZE - 2U) % UWB_DISTANCE_LPF_RECENT_SIZE);
    uint8_t i3 = (uint8_t)((f->idx + UWB_DISTANCE_LPF_RECENT_SIZE - 3U) % UWB_DISTANCE_LPF_RECENT_SIZE);

    int32_t d12 = f->recent[i1] - f->recent[i2];
    int32_t d23 = f->recent[i2] - f->recent[i3];

    return (uwb_abs_i32(d12) > eff_spike_thresh) &&
           (uwb_abs_i32(d23) > eff_spike_thresh) &&
           (((int64_t)d12 * (int64_t)d23) < 0);
}

static void uwb_distance_lpf_push_recent(uwb_distance_lpf_t *f, int32_t measurement_mm)
{
    f->recent[f->idx] = measurement_mm;
    f->idx = (uint8_t)((f->idx + 1U) % UWB_DISTANCE_LPF_RECENT_SIZE);
    if (f->count < UWB_DISTANCE_LPF_RECENT_SIZE) {
        f->count++;
    }
}

static bool uwb_distance_lpf_recent_stable_avg(const uwb_distance_lpf_t *f,
                                               int32_t *avg_out)
{
    if ((f == NULL) || (avg_out == NULL) ||
        (f->count < UWB_DISTANCE_LPF_STABLE_COUNT) ||
        (UWB_DISTANCE_LPF_STABLE_COUNT == 0U) ||
        (UWB_DISTANCE_LPF_STABLE_COUNT > UWB_DISTANCE_LPF_RECENT_SIZE)) {
        return false;
    }

    int32_t min_v = 2147483647L;
    int32_t max_v = -2147483647L;
    int64_t sum = 0;
    int32_t eff_stable_spread = (f->stable_spread_mm > 0L)
        ? f->stable_spread_mm : UWB_DISTANCE_LPF_STABLE_SPREAD_MM;

    for (uint8_t i = 0U; i < UWB_DISTANCE_LPF_STABLE_COUNT; i++) {
        uint8_t idx = (uint8_t)((f->idx + UWB_DISTANCE_LPF_RECENT_SIZE - 1U - i) %
                                UWB_DISTANCE_LPF_RECENT_SIZE);
        int32_t v = f->recent[idx];

        if (v < min_v) {
            min_v = v;
        }
        if (v > max_v) {
            max_v = v;
        }
        sum += (int64_t)v;
    }

    if ((max_v - min_v) > eff_stable_spread) {
        return false;
    }

    *avg_out = (int32_t)(sum / (int64_t)UWB_DISTANCE_LPF_STABLE_COUNT);
    return true;
}

static int32_t uwb_distance_lpf_calculate_wma_mm(const uwb_distance_lpf_t *f)
{
    int64_t weighted_sum = 0;
    int32_t weight_sum = 0;
    uint8_t n = f->count;

    if (n == 0U) {
        return f->estimate_mm;
    }

    if (n > UWB_DISTANCE_LPF_RECENT_SIZE) {
        n = UWB_DISTANCE_LPF_RECENT_SIZE;
    }

    for (uint8_t i = 0U; i < n; i++) {
        uint8_t idx = (uint8_t)((f->idx + UWB_DISTANCE_LPF_RECENT_SIZE - 1U - i) %
                                UWB_DISTANCE_LPF_RECENT_SIZE);

        /* i=0 is newest, so newest should get the largest weight. */
        int32_t weight = (int32_t)(n - i);

        weighted_sum += (int64_t)f->recent[idx] * (int64_t)weight;
        weight_sum += weight;
    }

    if (weight_sum == 0) {
        return f->estimate_mm;
    }

    return (int32_t)(weighted_sum / (int64_t)weight_sum);
}

static int32_t uwb_distance_lpf_dynamic_alpha_permille(const uwb_distance_lpf_t *f,
                                                       int32_t abs_delta_mm)
{
    int64_t alpha = (int64_t)f->base_alpha_permille + ((int64_t)abs_delta_mm * 2L);

    if (alpha < UWB_DISTANCE_LPF_MIN_ALPHA_PERMILLE) {
        alpha = UWB_DISTANCE_LPF_MIN_ALPHA_PERMILLE;
    }
    if (alpha > UWB_DISTANCE_LPF_MAX_ALPHA_PERMILLE) {
        alpha = UWB_DISTANCE_LPF_MAX_ALPHA_PERMILLE;
    }

    return (int32_t)alpha;
}

static bool uwb_distance_lpf_should_hard_reset(uwb_distance_lpf_t *f,
                                               int32_t abs_delta_mm,
                                               int32_t wma_mm,
                                               int32_t delta_mm)
{
    bool is_trend_break = (((int64_t)f->last_delta_mm * (int64_t)delta_mm) < 0) &&
                          (abs_delta_mm > UWB_DISTANCE_LPF_TREND_BREAK_MM);
    bool is_drifting = (uwb_abs_i32(f->estimate_mm - wma_mm) > UWB_DISTANCE_LPF_AVG_DRIFT_MM);

    f->last_delta_mm = delta_mm;
    return is_trend_break || is_drifting;
}

int32_t uwb_distance_lpf_update_mm(uwb_distance_lpf_t *f, int32_t measurement_mm)
{
    if (f == NULL) {
        return measurement_mm;
    }

    if ((measurement_mm < 0L) ||
        (measurement_mm > UWB_DISTANCE_LPF_MAX_MM) ||
        ((measurement_mm == 0L) && (f->estimate_mm > 100L))) {
        return f->estimate_mm;
    }

    if (f->initialized == 0U) {
        f->estimate_mm = measurement_mm;
        f->last_delta_mm = 0L;
        f->initialized = 1U;
        uwb_distance_lpf_push_recent(f, measurement_mm);
        return f->estimate_mm;
    }

    int32_t delta_mm = measurement_mm - f->estimate_mm;
    int32_t abs_delta_mm = uwb_abs_i32(delta_mm);
    int32_t eff_max_step = (f->max_step_mm > 0L)
        ? f->max_step_mm : UWB_DISTANCE_LPF_MAX_STEP_MM;
    int32_t eff_jump_thresh = (f->jump_threshold_mm > 0L)
        ? f->jump_threshold_mm : UWB_DISTANCE_LPF_JUMP_THRESHOLD_MM;
    int32_t eff_resync_delta = (f->resync_delta_mm > 0L)
        ? f->resync_delta_mm : UWB_DISTANCE_LPF_RESYNC_DELTA_MM;

    uwb_distance_lpf_push_recent(f, measurement_mm);

    int32_t stable_avg_mm = 0L;
    if (uwb_distance_lpf_recent_stable_avg(f, &stable_avg_mm)) {
        if (uwb_abs_i32(f->estimate_mm - stable_avg_mm) > eff_resync_delta) {
            f->estimate_mm = (int32_t)(((int64_t)f->estimate_mm * 3L +
                                        (int64_t)stable_avg_mm * 7L) / 10L);
            f->last_delta_mm = 0L;
            return f->estimate_mm;
        }
    }

    if (uwb_distance_lpf_is_spike(f)) {
        return f->estimate_mm;
    }

    if (abs_delta_mm > eff_jump_thresh) {
        int32_t step_mm = delta_mm;

        if (step_mm > eff_max_step) {
            step_mm = eff_max_step;
        } else if (step_mm < -eff_max_step) {
            step_mm = -eff_max_step;
        }

        f->estimate_mm += step_mm;
        f->last_delta_mm = step_mm;
        return f->estimate_mm;
    }

    int32_t wma_mm = uwb_distance_lpf_calculate_wma_mm(f);
    int32_t dynamic_alpha_permille = uwb_distance_lpf_dynamic_alpha_permille(f, abs_delta_mm);

    if (uwb_distance_lpf_should_hard_reset(f, abs_delta_mm, wma_mm, delta_mm)) {
        f->estimate_mm = (int32_t)(((int64_t)f->estimate_mm * 3L +
                                    (int64_t)wma_mm * 7L) / 10L);
        f->last_delta_mm = 0L;
        f->initialized = 1U;
        return f->estimate_mm;
    }

    int64_t retained = (int64_t)(1000L - dynamic_alpha_permille) * (int64_t)f->estimate_mm;
    int64_t injected = (int64_t)dynamic_alpha_permille * (int64_t)measurement_mm;
    f->estimate_mm = (int32_t)((retained + injected) / 1000L);

    return f->estimate_mm;
}

// 내부용: 슬라이딩 윈도우에 샘플 추가
void uwb_kf1d_reset(uwb_kf1d_t *kf)
{
    if (kf == NULL) return;
    kf->x[0] = 0.0f;
    kf->x[1] = 0.0f;
    kf->P[0][0] = 10000.0f;  /* large initial dist uncertainty */
    kf->P[0][1] = 0.0f;
    kf->P[1][0] = 0.0f;
    kf->P[1][1] = 10000.0f;  /* large initial vel uncertainty */
    kf->initialized = 0U;
}

static float uwb_kf_clamp_velocity(float vel_mm_s)
{
    if (vel_mm_s > UWB_KF_MAX_VEL_MM_S) {
        return UWB_KF_MAX_VEL_MM_S;
    }
    if (vel_mm_s < -UWB_KF_MAX_VEL_MM_S) {
        return -UWB_KF_MAX_VEL_MM_S;
    }
    return vel_mm_s;
}

float uwb_kf1d_update(uwb_kf1d_t *kf,
                      float measurement_mm,
                      float dt_s,
                      float los_conf)
{
    if (kf == NULL) {
        return isfinite(measurement_mm) ? measurement_mm : 0.0f;
    }

    if (!isfinite(measurement_mm)) {
        return (kf->initialized != 0U) ? kf->x[0] : 0.0f;
    }

    if (!isfinite(dt_s)) {
        dt_s = UWB_KF_DT_MIN_S;
    }

    if (!isfinite(los_conf)) {
        los_conf = 0.0f;
    }
    if (los_conf < 0.0f) los_conf = 0.0f;
    if (los_conf > 1.0f) los_conf = 1.0f;

    if (measurement_mm < 0.0f) {
        measurement_mm = 0.0f;
    }

    /* Clamp dt */
    if (dt_s < UWB_KF_DT_MIN_S) dt_s = UWB_KF_DT_MIN_S;
    if (dt_s > UWB_KF_DT_MAX_S) dt_s = UWB_KF_DT_MAX_S;

    /* Cold start: initialize with first measurement */
    if (kf->initialized == 0U) {
        kf->x[0] = measurement_mm;
        kf->x[1] = 0.0f;
        kf->initialized = 1U;
        return measurement_mm;
    }

    /* --- PREDICT --- */
    /* x_pred = F * x  where F = [[1, dt],[0, 1]] */
    float x1_pred = uwb_kf_clamp_velocity(kf->x[1] * UWB_KF_VEL_DAMPING);
    float x0_pred = kf->x[0] + x1_pred * dt_s;
    if (x0_pred < 0.0f) {
        x0_pred = 0.0f;
    }

    /* P_pred = F * P * F^T + Q */
    float dt2 = dt_s * dt_s;
    float P00_pred = kf->P[0][0]
                   + dt_s * (kf->P[1][0] + kf->P[0][1])
                   + dt2 * kf->P[1][1]
                   + UWB_KF_Q_DIST_MM2;
    float P01_pred = kf->P[0][1] + dt_s * kf->P[1][1];
    float P10_pred = kf->P[1][0] + dt_s * kf->P[1][1];
    float P11_pred = kf->P[1][1] + UWB_KF_Q_VEL_MM2S2;

    /* --- NLOS-scaled measurement noise --- */
    /* los_conf=1 -> R=R_base; los_conf=0 -> R=R_base*(1+NLOS_SCALE) */
    float nlos_factor = 1.0f + UWB_KF_R_NLOS_SCALE * (1.0f - los_conf);
    float R = UWB_KF_R_BASE_MM2 * nlos_factor;

    /* --- MAHALANOBIS GATE --- */
    float innovation = measurement_mm - x0_pred;
    float S = P00_pred + R;            /* innovation covariance */
    float gate = (innovation * innovation) / S;

    if (gate > UWB_KF_GATE_THRESHOLD) {
        /* Outlier: damp velocity and move slightly toward plausible measurement. */
        kf->x[0] = (0.85f * kf->x[0]) + (0.15f * measurement_mm);
        if (kf->x[0] < 0.0f) {
            kf->x[0] = 0.0f;
        }
        kf->x[1] = uwb_kf_clamp_velocity(x1_pred * UWB_KF_GATE_REJECT_VEL_DAMPING);
        kf->P[0][0] = P00_pred;
        kf->P[0][1] = P01_pred;
        kf->P[1][0] = P10_pred;
        kf->P[1][1] = P11_pred;
#if UWB_KF_DEBUG_LOGS
        printf("[UWB][KF][GATE] meas=%ld pred=%ld gate=%ld\r\n",
               (long)measurement_mm,
               (long)x0_pred,
               (long)gate);
#endif
        return kf->x[0];
    }

    /* --- UPDATE --- */
    /* Kalman gain: K = P_pred * H^T / S  where H = [1, 0] */
    float K0 = P00_pred / S;
    float K1 = P10_pred / S;

    kf->x[0] = x0_pred + K0 * innovation;
    kf->x[1] = x1_pred + K1 * innovation;
    if (fabsf(innovation) < UWB_KF_STOP_INNOV_MM) {
        kf->x[1] *= UWB_KF_STOP_VEL_DAMPING;
    }
    kf->x[1] = uwb_kf_clamp_velocity(kf->x[1]);
    if (kf->x[0] < 0.0f) {
        kf->x[0] = 0.0f;
    }

    /* P = (I - K*H) * P_pred */
    kf->P[0][0] = (1.0f - K0) * P00_pred;
    kf->P[0][1] = (1.0f - K0) * P01_pred;
    kf->P[1][0] = P10_pred - K1 * P00_pred;
    kf->P[1][1] = P11_pred - K1 * P01_pred;

    return kf->x[0];
}

static void twr_dist_filter_push(twr_dist_filter_t *f, float d_mm)
{
    f->buf[f->idx] = d_mm;
    f->idx = (uint8_t)((f->idx + 1U) % TWR_DIST_WINDOW_SIZE);
    if (f->count < TWR_DIST_WINDOW_SIZE) {
        f->count++;
    }
}

// 내부용: median + outlier 제거 후 평균 계산
//  - median_out : 윈도우 내 중앙값
//  - mean_out   : median 주변 ±THRESH 내 값들만 평균
static bool twr_dist_filter_compute_median_and_mean(
    const twr_dist_filter_t *f,
    float *median_out,
    float *mean_out)
{
    uint8_t n = f->count;
    if (n == 0U) {
        return false;
    }

    // 정렬용 임시 버퍼
    float tmp[TWR_DIST_WINDOW_SIZE];
    for (uint8_t i = 0U; i < n; i++) {
        tmp[i] = f->buf[i];
    }

    // 간단한 insertion sort (n이 작아서 충분함)
    for (uint8_t i = 1U; i < n; i++) {
        float  key = tmp[i];
        int8_t j   = (int8_t)i - 1;
        while ((j >= 0) && (tmp[(uint8_t)j] > key)) {
            tmp[(uint8_t)(j + 1)] = tmp[(uint8_t)j];
            j--;
        }
        tmp[(uint8_t)(j + 1)] = key;
    }

    float median = tmp[n / 2U];
    float sum    = 0.0f;
    uint8_t cnt  = 0U;

    // median에서 너무 벗어난 값(outlier)은 제외하고 평균
    for (uint8_t i = 0U; i < n; i++) {
        float d = f->buf[i];
        if (fabsf(d - median) <= TWR_DIST_OUTLIER_THRESH_MM) {
            sum += d;
            cnt++;
        }
    }

    if (cnt == 0U) {
        return false;
    }

    *median_out = median;
    *mean_out   = sum / (float)cnt;
    return true;
}

void twr_dist_filter_update_and_log_mm_ex(uint16_t anchor_id,
    uint8_t seq,
    float   dist_raw_mm,
    bool    quality_ok,
    uint8_t q_weight,
    const char *quality_fail_reason)
{
    twr_dist_filter_t *f = twr_get_filter_by_anchor(anchor_id);
    if (f == NULL) {
        return;
    }
#if defined(TAG_VERBOSE_LOGS) && TAG_VERBOSE_LOGS
    {
        long raw_l_local = uwb_round_mm(dist_raw_mm);
        printf("[TWR][TAG][DIST] AID=0x%04X SN=%u raw=%ld q=-1 clean_med=-1 iir=-1 mm\r\n",
               (unsigned int)anchor_id,
               (unsigned int)seq,
               raw_l_local);
    }
#endif

    float dist_med_mm = dist_raw_mm;
    float dist_iir_mm = dist_raw_mm;

    uint8_t used_for_filter = 0U;

    if (quality_ok) {
        // 1) 슬라이딩 윈도우에 raw 추가
        twr_dist_filter_push(f, dist_raw_mm);

        // 2) median + outlier 제거 후 평균
        float median = 0.0f;
        float mean_clean = 0.0f;

        if (twr_dist_filter_compute_median_and_mean(f, &median, &mean_clean)) {
        dist_med_mm = mean_clean;
        used_for_filter = 1U;
        } else {
        // 샘플이 너무 적은 초기 구간: med=raw 로 간주
        dist_med_mm = dist_raw_mm;
        used_for_filter = 1U;
        }

        // 3) IIR 필터 (초기구간 포함해서 항상 갱신 일관화)
        if (f->iir_initialized == 0U) {
            f->iir_initialized = 1U;
            f->iir_value       = dist_med_mm;
        } else {
            const float alpha_min = 0.05f;
            const float alpha_max = 0.25f;
            float alpha_eff = alpha_min + (alpha_max - alpha_min) * ((float)q_weight / 100.0f);
            f->iir_value = f->iir_value + alpha_eff * (dist_med_mm - f->iir_value);
        }
        
        dist_iir_mm = f->iir_value;
    } else {
        // 품질 불량: 필터 상태는 건드리지 않음 (push 금지)
        if (f->iir_initialized != 0U) {
            dist_iir_mm = f->iir_value; // 마지막 IIR 유지
        }

#if defined(TAG_VERBOSE_LOGS) && TAG_VERBOSE_LOGS
        long raw_l_temp = uwb_round_mm(dist_raw_mm);
        long q_l_temp   = -1L;
        long med_l_temp = -1L;
        long iir_l_temp = (f->iir_initialized != 0U)
            ? uwb_round_mm(dist_iir_mm) : -1L;

        printf("[TWR][FILT][SKIP] AID=0x%04X SN=%u raw=%ld q=%ld clean_med=%ld iir=%ld reason=%s\r\n",
            (unsigned int)anchor_id,
            (unsigned int)seq,
                raw_l_temp,
                q_l_temp,
                med_l_temp,
                iir_l_temp,
                quality_fail_reason);
#endif
            }

    // raw_l: always used for stats update or logging if enabled
    long raw_l = uwb_round_mm(dist_raw_mm);

#if defined(TAG_VERBOSE_LOGS) && TAG_VERBOSE_LOGS
    // q: 품질 통과면 raw 그대로, 아니면 -1
    long q_l = quality_ok ? raw_l : -1L;
#endif

    // med: quality_ok && used_for_filter일 때만 값, 아니면 -1
    long med_l = -1L;
    if (quality_ok && used_for_filter) {
        med_l = uwb_round_mm(dist_med_mm);
    }

    // iir: 초기화되어 있으면 값, 아니면 -1 (quality_ok=false여도 마지막값 출력)
    long iir_l = -1L;
    if (f->iir_initialized != 0U) {
        iir_l = uwb_round_mm(dist_iir_mm);
    }

#if defined(TAG_VERBOSE_LOGS) && TAG_VERBOSE_LOGS
    printf(
        "[TWR][TAG][DIST] AID=0x%04X SN=%u raw=%ld q=%ld clean_med=%ld iir=%ld mm\r\n",
        (unsigned int)anchor_id,
        (unsigned int)seq,
        raw_l,
        q_l,
        med_l,
        iir_l
    );
#endif

    // Ground-truth error stats (fixed jig): update only when we have valid filtered results.
    if (quality_ok && used_for_filter && (med_l >= 0L) && (iir_l >= 0L)) {
        twr_gt_stats_update((uint32_t)raw_l, (uint32_t)med_l, (uint32_t)iir_l);
    }
}


// Δ2: 4-arg 래퍼 함수 (ABI 호환성 유지)
void twr_dist_filter_update_and_log_mm(uint16_t anchor_id,
                                       uint8_t seq,
                                       float   dist_raw_mm,
                                       bool    quality_ok)
{
    twr_dist_filter_update_and_log_mm_ex(anchor_id, seq, dist_raw_mm, quality_ok, 100U, NULL);
}
