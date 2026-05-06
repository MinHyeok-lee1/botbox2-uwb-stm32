#ifndef UWB_QUALITY_H
#define UWB_QUALITY_H

#include <stdint.h>
#include <stdbool.h>
#include "deca_device_api.h"

// ============================================================================
// Diagnostics sanity check thresholds
// ============================================================================
#define TWR_DIAG_NOISE_MIN  2000U
#define TWR_DIAG_ACCUM_MIN  50U
#define TWR_DIAG_ACCUM_MAX  200U

// ============================================================================
// TWR quality scoring structures and API
// ============================================================================

// TWR 품질 점수 구조체
typedef struct {
    uint8_t  rssi_score;   // 0~100
    uint8_t  mp_score;     // 0~100 (멀티패스 품질)
    uint8_t  snr_score;    // 0~100
    uint16_t rssi_raw;     // 디버깅용 원시 값 (예: CIR_PWR 기반)
    uint16_t fp_amp_raw;   // 첫 경로 진폭
    uint16_t cir_pwr_raw;  // 채널 전력
    uint16_t noise_raw;    // 잡음 레벨
    // Phase 1: First Path 신뢰도 지표 (로그용)
    int16_t  idx_gap_q6;      // signed: peak_idx_q6 - fp_idx_q6
    uint32_t fp_idx_q6;       // raw for tuning (Q6)
    uint32_t peak_idx_q6;     // raw for tuning (Q6)
    uint16_t fp_to_peak_x100; // ONLY if same-domain amplitude is confirmed, else 0
    uint16_t fp_win_to_peak_x100; // (F1+F2+F3)*100/peak_amp (proxy)
    uint8_t  nlos_score;      // log-only heuristic (0~100)
    // MPDBG log fields (for output after function returns)
    uint32_t peak_idx;        // peak index (sample index)
    uint32_t d_q6;            // peak_idx_q6 - fp_idx_q6 (unsigned)
    uint32_t peak_amp;        // peak amplitude
    uint32_t fp_est;          // max(F1,F2,F3)
    uint32_t amp_ratio_x100;  // (peak_amp * 100) / (fp_est + 1)
    long     mp_raw;          // unclamped mp score (for debugging)
} twr_quality_t;

// Public API
bool twr_diag_is_sane(const dwt_rxdiag_t *d);
twr_quality_t twr_quality_from_diag(const dwt_rxdiag_t *d);
uint8_t twr_quality_weight_u8(long dist_raw_mm_l, uint16_t rssi_raw_u16,
                              uint8_t snr_u8, uint16_t fp_amp_u16,
                              uint16_t gap_q6_u16, long iir_mm_l,
                              uint8_t filt_samples, const char **out_reason);

#endif // UWB_QUALITY_H
