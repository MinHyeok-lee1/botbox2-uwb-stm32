#ifndef UWB_FILTER_H
#define UWB_FILTER_H

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

// ============================================================================
// TWR distance filtering Constants
// ============================================================================

// ===== 튜닝 파라미터 =====
#define TWR_DIST_WINDOW_SIZE         7      // 슬라이딩 윈도우 크기
#define TWR_DIST_OUTLIER_THRESH_MM   80.0f  // median에서 ±80mm 이상이면 outlier
#define TWR_DIST_IIR_ALPHA           0.2f   // IIR 필터 계수 (0.1~0.3 추천)

#define TWR_GT_ENABLE      1U      // 1: enable ground-truth error stats (fixed jig), 0: disable
#define TWR_GT_TARGET_MM   300U    // set to your jig ground-truth distance in mm
#define TWR_GT_PRINT_EVERY 100U    // print stats every N samples

#define UWB_DISTANCE_LPF_RECENT_SIZE 5U
#define UWB_DISTANCE_LPF_MAX_MM      30000L

#define UWB_DISTANCE_LPF_MAX_ALPHA_PERMILLE  350L
#define UWB_DISTANCE_LPF_MIN_ALPHA_PERMILLE   50L
#define UWB_DISTANCE_LPF_BASE_ALPHA_PERMILLE 200L

#define UWB_DISTANCE_LPF_JUMP_THRESHOLD_MM    200L
#define UWB_DISTANCE_LPF_TREND_BREAK_MM       100L
#define UWB_DISTANCE_LPF_AVG_DRIFT_MM         80L
#define UWB_DISTANCE_LPF_SPIKE_THRESHOLD_MM   80L
#define UWB_DISTANCE_LPF_MAX_STEP_MM          80L

/*
 * Recovery path for a stale estimate after multipath / FP-glitch events.
 * If the newest raw samples become stable again but the11 current estimate is
 * still far away, snap the estimate back to the recent raw average.
 */
#define UWB_DISTANCE_LPF_STABLE_COUNT         4U
#define UWB_DISTANCE_LPF_STABLE_SPREAD_MM     180L
#define UWB_DISTANCE_LPF_RESYNC_DELTA_MM      180L


// ============================================================================
// TWR distance filtering (quality flag + median/outlier + IIR smoothing)
// RULES:
// - 모든 printf/printf는 32bit 정수만 사용 (%ld/%lu).
// - 거리는 모두 mm 단위.
// - quality_ok == false 일 때는 필터 상태를 건드리지 않고 raw + 이전 IIR만 로그.
// ============================================================================

typedef struct
{
    float   buf[TWR_DIST_WINDOW_SIZE];
    uint8_t count;           // 현재까지 쌓인 샘플 수 (최대 WINDOW_SIZE)
    uint8_t idx;             // 다음으로 쓸 인덱스
    uint8_t _pad;
    float   iir_value;       // 마지막 IIR 결과
    uint8_t iir_initialized; // 0: 미초기화, 1: 초기화됨
} twr_dist_filter_t;

typedef struct
{
    int32_t recent[UWB_DISTANCE_LPF_RECENT_SIZE];
    int32_t estimate_mm;
    int32_t last_delta_mm;
    int32_t base_alpha_permille;
    uint8_t count;
    uint8_t idx;
    uint8_t initialized;
} uwb_distance_lpf_t;

static inline long uwb_round_mm(float mm) {
    return (long)((mm >= 0.0f) ? (mm + 0.5f) : (mm - 0.5f));
}

// 외부(main.c 등)에 구현될 필터 매핑 헬퍼
extern twr_dist_filter_t* twr_get_filter_by_anchor(uint16_t anchor_id);

void twr_reason_add(char* buf, size_t buf_sz, const char* token);
void twr_dist_filter_reset(twr_dist_filter_t *f);

void uwb_distance_lpf_reset(uwb_distance_lpf_t *f);
int32_t uwb_distance_lpf_update_mm(uwb_distance_lpf_t *f, int32_t measurement_mm);

void twr_dist_filter_update_and_log_mm_ex(uint16_t anchor_id,
    uint8_t seq,
    float   dist_raw_mm,
    bool    quality_ok,
    uint8_t q_weight,
    const char *quality_fail_reason);

void twr_dist_filter_update_and_log_mm(uint16_t anchor_id,
                                       uint8_t seq,
                                       float   dist_raw_mm,
                                       bool    quality_ok);

#endif // UWB_FILTER_H
