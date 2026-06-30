#include "uwb_quality.h"
#include <string.h>
#include <math.h>

// ============================================================================
// TWR quality scoring from DW3000 RX diagnostics
// ============================================================================

// Δ1: DIAG sanity check helper
bool twr_diag_is_sane(const dwt_rxdiag_t *d)
{
    // ipatovFpIndex != 0
    if (d->ipatovFpIndex == 0U) {
        return false;
    }
    
    // ipatovPower != 0
    uint32_t cir_pwr = (d->ipatovPower & 0x1FFFFUL);
    if (cir_pwr == 0U) {
        return false;
    }
    
    // ipatovF1 (noise proxy)가 너무 작으면 invalid 처리
    uint32_t noise_proxy = (d->ipatovF1 & 0x3FFFFFUL);
    if (noise_proxy < TWR_DIAG_NOISE_MIN) {
        return false;
    }
    
    // ipatovAccumCount 범위 체크
    uint32_t accum = (uint32_t)d->ipatovAccumCount;
    if (accum < TWR_DIAG_ACCUM_MIN  || accum > TWR_DIAG_ACCUM_MAX) {
        return false;
    }
    
    return true;
}

uint8_t twr_quality_weight_u8(long dist_raw_mm_l,
                                     uint16_t rssi_raw_u16,
                                     uint8_t snr_u8,
                                     uint16_t fp_amp_u16,
                                     uint16_t gap_q6_u16,
                                     long iir_mm_l,
                                     uint8_t filt_samples,
                                     const char **out_reason)
{
    uint8_t w = 100U;
    *out_reason = "ok";

    // rssi raw
    if (rssi_raw_u16 < 22U) {
        w = (w > 25U) ? (uint8_t)(w - 25U) : 0U;
        *out_reason = "rssi_raw_low";
    }

    // fp amplitude
    if (fp_amp_u16 < 1000U) {
        w = (w > 30U) ? (uint8_t)(w - 30U) : 0U;
        *out_reason = "fp_amp_low";
    }

    // snr
    if (snr_u8 < 8U) {
        w = (w > 25U) ? (uint8_t)(w - 25U) : 0U;
        *out_reason = "snr_low";
    }

    // gap
    if (gap_q6_u16 > 200U) {
        w = (w > 20U) ? (uint8_t)(w - 20U) : 0U;
        *out_reason = "gap_high";
    }

    // jump (IIR 존재 시만)
    if (iir_mm_l >= 0L && filt_samples > 0U) {
        long jump_abs = (dist_raw_mm_l > iir_mm_l) ? (dist_raw_mm_l - iir_mm_l) : (iir_mm_l - dist_raw_mm_l);

        if (jump_abs > 700L) {
            w = (w > 30U) ? (uint8_t)(w - 30U) : 0U;
            *out_reason = "jump_abs";
        }

        if (iir_mm_l > 0L) {
            long jump_rel_x100 = (jump_abs * 100L) / iir_mm_l;
            if (jump_rel_x100 > 40L) {
                w = (w > 20U) ? (uint8_t)(w - 20U) : 0U;
                *out_reason = "jump_rel";
            }
        }
    }

    // floor: 완전 폐기 대신 약반영
    if (w < 20U) w = 20U;
    return w;
}



// DW3000 RX diagnostics를 읽어서 품질 점수(0~100)로 변환
twr_quality_t twr_quality_from_diag(const dwt_rxdiag_t *d)
{
    twr_quality_t q;
    memset(&q, 0, sizeof(q));

    // Δ1: DIAG sanity check
    bool diag_ok = twr_diag_is_sane(d);

    // ipatovPeak에서 amplitude 추출: [20:0] 비트
    uint32_t fp_amp = (d->ipatovPeak & 0x1FFFFFUL);
    // ipatovPower: [16:0] 채널 전력
    uint32_t cir_pwr = (d->ipatovPower & 0x1FFFFUL);
    // ipatovF1을 noise proxy로 사용 (실제로는 다른 필드일 수 있음)
    uint32_t noise_proxy = (d->ipatovF1 & 0x3FFFFFUL);
    if (noise_proxy == 0U) {
        noise_proxy = 1U;  // 0으로 나누기 방지
    }

    // 0. 원시 값 저장 (diag_ok와 관계없이 항상 채움)
    q.fp_amp_raw  = (uint16_t)(fp_amp > 65535U ? 65535U : fp_amp);
    q.cir_pwr_raw = (uint16_t)(cir_pwr > 65535U ? 65535U : cir_pwr);
    q.noise_raw   = (uint16_t)(noise_proxy > 65535U ? 65535U : noise_proxy);
    q.rssi_raw    = q.cir_pwr_raw;  // cir_pwr를 RSSI proxy로 사용

    // Δ1: diag_ok가 false이면 점수는 0으로 두고 return (MPDBG 로그는 출력하지 않음)
    if (!diag_ok) {
        q.rssi_score = 0;
        q.mp_score = 0;
        q.snr_score = 0;
        return q;
    }

    // 1) RSSI 점수 (0~100) : cir_pwr 기반 간단 스케일링
    {
        uint32_t rssi_proxy = cir_pwr;
#if defined(UWB_RF_CHANNEL) && (UWB_RF_CHANNEL == 9)
        const uint32_t RSSI_MIN = 50U;   // field cir min observed: ~56
        const uint32_t RSSI_MAX = 110U;  // field cir max observed: ~100, with margin
#else
        const uint32_t RSSI_MIN = 20U;   // CH5 tuned range.
        const uint32_t RSSI_MAX = 55U;
#endif

        uint8_t score = 0;
        if (rssi_proxy <= RSSI_MIN) {
            score = 0;
        } else if (rssi_proxy >= RSSI_MAX) {
            score = 100;
        } else {
            uint32_t num = (rssi_proxy - RSSI_MIN) * 100U;
            uint32_t den = (RSSI_MAX - RSSI_MIN);
            score = (uint8_t)(num / den);
        }
        q.rssi_score = score;
    }

    // 2) 멀티패스 점수 (0~100): 100 = clean LoS, 0 = heavy multipath
    {
        uint32_t peak_amp = (uint32_t)(d->ipatovPeak & 0x1FFFFFU);
        uint32_t peak_idx = (uint32_t)((d->ipatovPeak >> 21) & 0x3FFU);

        uint32_t fp_idx_raw = (uint32_t)d->ipatovFpIndex;

        // fp_est from FP+1..FP+3 (22b)
        uint32_t f1 = (uint32_t)(d->ipatovF1 & 0x3FFFFFU);
        uint32_t f2 = (uint32_t)(d->ipatovF2 & 0x3FFFFFU);
        uint32_t f3 = (uint32_t)(d->ipatovF3 & 0x3FFFFFU);
        uint32_t fp_est = f1;
        if (f2 > fp_est) fp_est = f2;
        if (f3 > fp_est) fp_est = f3;

        // amp ratio vs fp_est (NOT cir_pwr)
        uint32_t amp_ratio_x100 = (uint32_t)((peak_amp * 100UL) / (fp_est + 1UL));

        uint32_t peak_idx_q6 = ((uint32_t)peak_idx << 6);
        uint32_t d_q6 = (peak_idx_q6 > fp_idx_raw) ? (peak_idx_q6 - fp_idx_raw) : (fp_idx_raw - peak_idx_q6);

        int mp = 100;

        // smooth index mismatch penalty: (d_q6/64)*8 with rounding
        mp -= (int)((d_q6 * 8UL + 32UL) / 64UL);

        if (amp_ratio_x100 > 140UL) {
            mp -= (int)((amp_ratio_x100 - 140UL) * 2UL);
        }

        // Δ2: mp_raw (클램프 전) 보관 + 동일 수식 기반 클램프
        long mp_raw = (long)mp;
        unsigned mp_clamped = 0U;
        if (mp_raw <= 0L) {
            mp_clamped = 0U;
        } else if (mp_raw >= 100L) {
            mp_clamped = 100U;
        } else {
            mp_clamped = (unsigned)mp_raw;
        }
        q.mp_score = (uint8_t)mp_clamped;
        q.mp_raw   = mp_raw;

        // Phase 1: First Path 신뢰도 지표 계산 (로그용)
        // fp_idx_q6, peak_idx_q6 저장
        q.fp_idx_q6 = fp_idx_raw;
        q.peak_idx_q6 = peak_idx_q6;

        // MPDBG log fields 저장 (for output after function returns)
        q.peak_idx = peak_idx;
        q.d_q6 = d_q6;
        q.peak_amp = peak_amp;
        q.fp_est = fp_est;
        q.amp_ratio_x100 = amp_ratio_x100;

        // idx_gap_q6: signed gap (peak_idx_q6 - fp_idx_q6)
        int32_t gap_q6_raw = (int32_t)peak_idx_q6 - (int32_t)fp_idx_raw;
        // clamp to int16_t range
        if (gap_q6_raw > 32767) gap_q6_raw = 32767;
        else if (gap_q6_raw < -32768) gap_q6_raw = -32768;
        q.idx_gap_q6 = (int16_t)gap_q6_raw;

        // fp_win: F1+F2+F3 (if present, else F1 only)
        uint32_t fp_win = f1;
        if (f2 > 0U || f3 > 0U) {
            fp_win = f1 + f2 + f3;
        }

        // fp_win_to_peak_x100: (fp_win*100)/peak_amp
        // Note: peak_amp (from ipatovPeak[20:0]) and fp_win (F1+F2+F3 from ipatovF1/F2/F3[21:0])
        // are in the same amplitude domain (~10k~20k range), unlike ipatovPower/cir which are small (tens).
        // Using peak_amp as denominator ensures same-domain ratio calculation.
        uint32_t denom_same_domain = peak_amp;
        if (denom_same_domain > 0U) {
            uint32_t fp_win_ratio = (fp_win * 100UL) / denom_same_domain;
            // Clamp to 0..100 (per requirement: fpwin2peak must be 0..100)
            if (fp_win_ratio > 100U) fp_win_ratio = 100U;
            q.fp_win_to_peak_x100 = (uint16_t)fp_win_ratio;
        } else {
            q.fp_win_to_peak_x100 = 0U;
        }

        /* fp_to_peak_x100: fp_est (max of F1/F2/F3) vs peak_amp ratio.
         * Both are in the same CIR amplitude domain. Ratio < 70 means
         * the strongest path is well above the first-path window -> NLOS signal. */
        if (peak_amp > 0U) {
            uint32_t fp_to_peak = (fp_est * 100UL) / peak_amp;
            if (fp_to_peak > 100U) fp_to_peak = 100U;
            q.fp_to_peak_x100 = (uint16_t)fp_to_peak;
        } else {
            q.fp_to_peak_x100 = 0U;
        }

        // nlos_score: log-only heuristic (using now-meaningful fpwin2peak)
        uint32_t nlos = 0U;
        if (q.idx_gap_q6 > 64) {
            nlos += 30U;
        }
        if (q.fp_win_to_peak_x100 < 35U) {
            nlos += 40U;  // threshold can be tuned later
         }
        if (q.fp_to_peak_x100 != 0U && q.fp_to_peak_x100 < 70U) {
            nlos += 30U;
        }
        q.nlos_score = (uint8_t)(nlos > 100U ? 100U : nlos);
    }

    // 3) SNR 점수 (0~100)
    {
        uint8_t score = 0;
        if (noise_proxy > 0U) {
            float snr = (float)fp_amp / (float)noise_proxy; // FP / NOISE
            if (snr < 0.0f) snr = 0.0f;
            if (snr > 10.0f) snr = 10.0f;

            // snr = 0 → 0점, snr = 10 → 100점
            float norm = snr / 10.0f;
            if (norm > 1.0f) norm = 1.0f;

            int s = (int)(norm * 100.0f + 0.5f);
            if (s < 0)   s = 0;
            if (s > 100) s = 100;
            score = (uint8_t)s;
        } else {
            score = 0;
        }

        q.snr_score = score;
    }

    return q;
}
