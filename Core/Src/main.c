/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "fdcan.h"
#include "fmac.h"
#include "usart.h"
#include "spi.h"
#include "tim.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "uwb_filter.h"
#include "uwb_quality.h"
#include "uwb_cfg.h"
#include "deca_device_api.h"
#include "deca_regs.h"
#include "stm32g4xx.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <stdbool.h>

/* DW3000 status bit compatibility fallbacks */
#ifndef SYS_STATUS_AFFREJ_BIT_MASK
#define SYS_STATUS_AFFREJ_BIT_MASK SYS_STATUS_ARFE_BIT_MASK
#endif
#ifndef SYS_STATUS_RXERR_ALL_MASK
#define SYS_STATUS_RXERR_ALL_MASK SYS_STATUS_ALL_RX_ERR
#endif
#ifndef SYS_STATUS_RXTO_ALL_MASK
#define SYS_STATUS_RXTO_ALL_MASK SYS_STATUS_ALL_RX_TO
#endif
#include <stdlib.h>
#include <math.h>

// ===== 튜닝 파라미터 =====
#define POLL_TX_TO_RESP_RX_DLY_UUS 900
#define RESP_RX_TIMEOUT_UUS         1500  
#define POLL_RX_TO_RESP_TX_DLY_UUS   1400   

#define TX_ANT_DLY 16385
#define RX_ANT_DLY 16385





#define TWR_FP_MAX_GAP_Q6 200
#define FP_GAP_MAX_SAMP 12U

#define TWR_QUAL_MIN_RSSI   5U
#define TWR_QUAL_WARN_RSSI  TWR_QUAL_MIN_RSSI  // RSSI warning threshold (does NOT gate ok)

#define TWR_QUAL_MIN_SNR    7U
#define TWR_QUAL_MIN_MP     50U

// ===== 기타 파라미터 =====
#define RESP_TS_ADJ_UNITS  0   // 프레임에 넣는 resp_tx_ts 보정치(유닛)
#define RXFLEN_MASK    0x0000007FUL    /* Receive Frame Length (0 to 127) */

// system_config_t is now replaced by uwb_cfg_t provided in uwb_cfg.h

/* -------------------------------------------------------------------------- */
/* Optional project-level config header                                       */
/* Enable with: -DUWB_USE_USER_CFG_FILE                                       */
/* Header name:  uwb_user_cfg.h                                               */
/* Priority: build option > uwb_user_cfg.h > hard default                     */
/* -------------------------------------------------------------------------- */
#if defined(UWB_USE_USER_CFG_FILE)
#include "uwb_user_cfg.h"
#endif

/* -------------------------------------------------------------------------- */
/* UWB ranging mode selection (compile-time dispatcher)                       */
/* 0: SS-TWR, 1: DS-TWR                                                       */
/* -------------------------------------------------------------------------- */
// Fallback macros are now natively handled by uwb_cfg_get_default() in uwb_cfg.c

/* Redundant UWB_DEFAULT_* macros removed. Now using shared macros in uwb_cfg.h */

#ifndef UWB_PAN_ID
# ifdef UWB_CFG_FILE_PAN_ID
#  define UWB_PAN_ID UWB_CFG_FILE_PAN_ID
# else
#  define UWB_PAN_ID UWB_DEFAULT_PAN_ID
# endif
#endif

#ifndef UWB_TAG_SHORT_ADDR
# ifdef UWB_CFG_FILE_TAG_SHORT_ADDR
#  define UWB_TAG_SHORT_ADDR UWB_CFG_FILE_TAG_SHORT_ADDR
# else
#  define UWB_TAG_SHORT_ADDR UWB_DEFAULT_TAG_SHORT_ADDR
# endif
#endif

#ifndef UWB_ANCHOR_BASE_ADDR
# ifdef UWB_CFG_FILE_ANCHOR_BASE_ADDR
#  define UWB_ANCHOR_BASE_ADDR UWB_CFG_FILE_ANCHOR_BASE_ADDR
# else
#  define UWB_ANCHOR_BASE_ADDR UWB_DEFAULT_ANCHOR_BASE_ADDR
# endif
#endif

#ifndef UWB_ANCHOR_LEFT_SHORT_ADDR
# ifdef UWB_CFG_FILE_ANCHOR_LEFT_SHORT_ADDR
#  define UWB_ANCHOR_LEFT_SHORT_ADDR UWB_CFG_FILE_ANCHOR_LEFT_SHORT_ADDR
# else
#  define UWB_ANCHOR_LEFT_SHORT_ADDR (UWB_ANCHOR_BASE_ADDR + 0U)
# endif
#endif

#ifndef UWB_ANCHOR_RIGHT_SHORT_ADDR
# ifdef UWB_CFG_FILE_ANCHOR_RIGHT_SHORT_ADDR
#  define UWB_ANCHOR_RIGHT_SHORT_ADDR UWB_CFG_FILE_ANCHOR_RIGHT_SHORT_ADDR
# else
#  define UWB_ANCHOR_RIGHT_SHORT_ADDR (UWB_ANCHOR_BASE_ADDR + 1U)
# endif
#endif

#ifndef UWB_ANCHOR_REAR_SHORT_ADDR
# ifdef UWB_CFG_FILE_ANCHOR_REAR_SHORT_ADDR
#  define UWB_ANCHOR_REAR_SHORT_ADDR UWB_CFG_FILE_ANCHOR_REAR_SHORT_ADDR
# else
#  define UWB_ANCHOR_REAR_SHORT_ADDR (UWB_ANCHOR_BASE_ADDR + 2U)
# endif
#endif

#ifndef UWB_ANCHOR_SHORT_ADDR
# ifdef UWB_CFG_FILE_ANCHOR_SHORT_ADDR
#  define UWB_ANCHOR_SHORT_ADDR UWB_CFG_FILE_ANCHOR_SHORT_ADDR
# else
#  define UWB_ANCHOR_SHORT_ADDR UWB_DEFAULT_ANCHOR_SHORT_ADDR
# endif
#endif

#ifndef UWB_RANGING_MODE
# ifdef UWB_CFG_FILE_RANGING_MODE
#  define UWB_RANGING_MODE UWB_CFG_FILE_RANGING_MODE
# else
#  define UWB_RANGING_MODE UWB_DEFAULT_RANGING_MODE
# endif
#endif

#ifndef UWB_FILTER_PROFILE
# ifdef UWB_CFG_FILE_FILTER_PROFILE
#  define UWB_FILTER_PROFILE UWB_CFG_FILE_FILTER_PROFILE
# else
#  define UWB_FILTER_PROFILE UWB_DEFAULT_FILTER_PROFILE
# endif
#endif

#ifndef UWB_COMM_MODE
# ifdef UWB_CFG_FILE_COMM_MODE
#  define UWB_COMM_MODE UWB_CFG_FILE_COMM_MODE
# else
#  define UWB_COMM_MODE UWB_DEFAULT_COMM_MODE
# endif
#endif

#ifndef UWB_ANCHOR_COUNT
# ifdef UWB_CFG_FILE_ANCHOR_COUNT
#  define UWB_ANCHOR_COUNT UWB_CFG_FILE_ANCHOR_COUNT
# else
#  define UWB_ANCHOR_COUNT UWB_DEFAULT_ANCHOR_COUNT
# endif
#endif

#ifndef UWB_ANCHOR_0
# ifdef UWB_CFG_FILE_ANCHOR_0
#  define UWB_ANCHOR_0 UWB_CFG_FILE_ANCHOR_0
# else
#  define UWB_ANCHOR_0 (UWB_ANCHOR_BASE_ADDR + 0U)
# endif
#endif

#ifndef UWB_ANCHOR_1
# ifdef UWB_CFG_FILE_ANCHOR_1
#  define UWB_ANCHOR_1 UWB_CFG_FILE_ANCHOR_1
# else
#  define UWB_ANCHOR_1 (UWB_ANCHOR_BASE_ADDR + 1U)
# endif
#endif

#ifndef UWB_ANCHOR_2
# ifdef UWB_CFG_FILE_ANCHOR_2
#  define UWB_ANCHOR_2 UWB_CFG_FILE_ANCHOR_2
# else
#  define UWB_ANCHOR_2 (UWB_ANCHOR_BASE_ADDR + 2U)
# endif
#endif

/* -------------------------------------------------------------------------- */
/* Validation                                                                 */
/* -------------------------------------------------------------------------- */
#if ((UWB_RANGING_MODE) != UWB_RANGING_MODE_SS) && ((UWB_RANGING_MODE) != UWB_RANGING_MODE_DS)
#error "UWB_RANGING_MODE must be 0(SS) or 1(DS)"
#endif

#if ((UWB_FILTER_PROFILE) < FILTER_PROFILE_WEAK) || ((UWB_FILTER_PROFILE) > FILTER_PROFILE_STRONG)
#error "UWB_FILTER_PROFILE must be 0(WEAK)..2(STRONG)"
#endif

#if ((UWB_COMM_MODE) != COMM_MODE_ANYCAST) && ((UWB_COMM_MODE) != COMM_MODE_MULTICAST)
#error "UWB_COMM_MODE must be 0(ANYCAST) or 1(MULTICAST)"
#endif

// UWB_ANCHOR_COUNT legacy check: unified to 1..8 to match uwb_cfg.h
#if ((UWB_ANCHOR_COUNT) < 1) || ((UWB_ANCHOR_COUNT) > 8)
#error "UWB_ANCHOR_COUNT must be 1..8"
#endif

#if ((UWB_ANCHOR_COUNT) >= 1) && ((UWB_ANCHOR_0) != ((UWB_ANCHOR_BASE_ADDR) + 0U))
#error "UWB_ANCHOR_0 must be UWB_ANCHOR_BASE_ADDR + 0"
#endif

#if ((UWB_ANCHOR_COUNT) >= 2) && ((UWB_ANCHOR_1) != ((UWB_ANCHOR_BASE_ADDR) + 1U))
#error "UWB_ANCHOR_1 must be UWB_ANCHOR_BASE_ADDR + 1 when UWB_ANCHOR_COUNT >= 2"
#endif

#if ((UWB_ANCHOR_COUNT) >= 3) && ((UWB_ANCHOR_2) != ((UWB_ANCHOR_BASE_ADDR) + 2U))
#error "UWB_ANCHOR_2 must be UWB_ANCHOR_BASE_ADDR + 2 when UWB_ANCHOR_COUNT >= 3"
#endif


/* ---------- role ---------- */
#if defined(UWB_ROLE_TX)
  #define THIS_ROLE_STR   "TX"
#elif defined(UWB_ROLE_RX)
  #define THIS_ROLE_STR   "RX"
#else
  #define THIS_ROLE_STR   "UNK"
#endif

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

// twr_dist_filter_t is now provided by uwb_filter.h

// twr_quality_t is now provided by uwb_quality.h

// TAG 쪽에서 사용하는 전역 필터 상태 (앵커별 분리)
#ifndef NUM_OF_ANCHORS
#define NUM_OF_ANCHORS 3U
#endif
static twr_dist_filter_t g_twr_dist_filter_by_anchor[NUM_OF_ANCHORS];

// anchor_id -> per-anchor filter 매핑 (구현은 g_sys_cfg 선언 이후에 정의)
static int8_t twr_anchor_slot_from_id(uint16_t anchor_id);
twr_dist_filter_t* twr_get_filter_by_anchor(uint16_t anchor_id);

// 내부용: 필터 상태 초기화

// Filter functions (twr_reason_add, twr_dist_filter_push, twr_dist_filter_compute_median_and_mean,
// twr_dist_filter_update_and_log_mm_ex, twr_dist_filter_update_and_log_mm, twr_gt_stats_*)
// are now provided by uwb_filter.c

// 앱 초기화 시 한 번 호출 (g_twr_dist_filter_by_anchor는 main.c 로컬)
void twr_dist_filter_init(void)
{
    for (uint8_t i = 0U; i < NUM_OF_ANCHORS; i++) {
        twr_dist_filter_reset(&g_twr_dist_filter_by_anchor[i]);
    }
}

// Quality scoring functions (twr_diag_is_sane, twr_quality_weight_u8,
// twr_quality_from_diag) are now provided by uwb_quality.c
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

uint64_t get_tx_timestamp_u64(void)
{
    uint8_t ts_tab[5];
    uint64_t ts = 0;
    int8_t i;
    dwt_readtxtimestamp(ts_tab);
    for (i = 4; i >= 0; i--) {
        ts <<= 8;
        ts |= ts_tab[i];
    }
    return ts;
}

uint64_t get_rx_timestamp_u64(void)
{
    uint8_t ts_tab[5];
    uint64_t ts = 0;
    int8_t i;
    dwt_readrxtimestamp(ts_tab);
    for (i = 4; i >= 0; i--) {
        ts <<= 8;
        ts |= ts_tab[i];
    }
    return ts;
}
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
/* ---- Distance filter (integer IIR low-pass) ---------------------------------
 * y[n] = y[n-1] + alpha * (x[n] - y[n-1])
 * Implemented as fixed-point: alpha = LPF_K / (1<<LPF_SHIFT)
 * - No floats
 * - Safe with newlib-nano printf rules
 */
#define DIST_LPF_SHIFT   (4u)   /* 1<<4 = 16 */
#define DIST_LPF_K       (2u)   /* alpha = 2/16 = 0.125 */

static int32_t  g_dist_filt_mm      = 0;
static uint8_t  g_dist_filt_inited  = 0;

static inline int32_t dist_lpf_update_mm(int32_t x_mm)
{
    if (!g_dist_filt_inited) {
        g_dist_filt_mm = x_mm;
        g_dist_filt_inited = 1;
        return g_dist_filt_mm;
    }

    int32_t err = x_mm - g_dist_filt_mm;
    /* rounding: add 0.5 LSB before shift when positive/negative */
    int32_t delta = (err * (int32_t)DIST_LPF_K);
    if (delta >= 0) delta += (1 << (DIST_LPF_SHIFT - 1));
    else            delta -= (1 << (DIST_LPF_SHIFT - 1));
    g_dist_filt_mm += (delta >> DIST_LPF_SHIFT);

    return g_dist_filt_mm;
}

static inline void dist_lpf_reset(void)
{
    g_dist_filt_inited = 0;
    g_dist_filt_mm = 0;
}
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
extern TIM_HandleTypeDef htim2;

/* ---------- micros (TIM2) ---------- */
static inline void micros_init(void)
{
    HAL_TIM_Base_Start(&htim2);
}

uint32_t micros(void)
{
    return __HAL_TIM_GET_COUNTER(&htim2);
}

static inline void micros_delay(uint32_t us)
{
    const uint32_t start = micros();
    while ((uint32_t)(micros() - start) < us) {
        // busy-wait
        // 필요하면 전력/인터럽트 친화적으로:
        // __NOP();
        // 또는 __WFI();  (단, 인터럽트가 주기적으로 들어오는 환경에서만 추천)
    }
}

/* ===== DW3000 pin mapping ===== */
#define DW3000_CS_PORT     GPIOB
#define DW3000_CS_PIN      GPIO_PIN_6

#define DW3000_RST_PORT    GPIOA
#define DW3000_RST_PIN     GPIO_PIN_8
/* ========================================= */

static void dw3000_reset(void)
{
    /* Active-low reset (open-drain) */
    HAL_GPIO_WritePin(DW3000_RST_PORT, DW3000_RST_PIN, GPIO_PIN_RESET);
    HAL_Delay(2);   // ≥1 ms
    HAL_GPIO_WritePin(DW3000_RST_PORT, DW3000_RST_PIN, GPIO_PIN_SET);
    HAL_Delay(2);
}

typedef enum {
  UWB_OK = 0,                ///< Initialization successful

  UWB_ERROR_CONNECT = -1,    ///< Device not detected or wrong device ID
  UWB_ERROR_TIMEOUT = -2,    ///< Timeout while waiting for IDLE/RC
  UWB_ERROR_INIT = -3,       ///< DW3000 initialization failed
} uwb_error_codes;

static uwb_error_codes uwb_lib_init(void)
{
    uint32_t dev_id = dwt_readdevid();

    if (dwt_check_dev_id() == DWT_ERROR) {
        printf("[STEP3] Unknown device id: 0x%08lX\r\n", (unsigned long)dev_id);
        return UWB_ERROR_CONNECT;
    }

    printf("[STEP3] Device id: 0x%08lX\r\n", (unsigned long)dev_id);

    int cnt = 1000;
    while (!dwt_checkidlerc()) {
        if (cnt <= 0) {
            printf("[STEP3] Idle failed\r\n");
            return UWB_ERROR_TIMEOUT;
        }
        cnt--;
        deca_sleep(1);
    }

    if (dwt_initialise(DWT_DW_INIT) == DWT_ERROR) {
        printf("[STEP3] Init failed\r\n");
        return UWB_ERROR_INIT;
    }

    return UWB_OK;
}

static dwt_config_t default_config = {
  5, DWT_PLEN_128, DWT_PAC8, 9, 9, 1,
  DWT_BR_6M8, DWT_PHRMODE_STD, DWT_PHRRATE_STD,
  (129 + 8 - 8),
  DWT_STS_MODE_OFF, DWT_STS_LEN_64, DWT_PDOA_M0
};

uwb_error_codes uwb_lib_cfg(dwt_config_t* config);

dwt_txconfig_t txconfig_ch5 = {
  0x34,       /* PG delay. */
  0xfdfdfdfd, /* TX power. */
  0x0         /*PG count*/
};

dwt_txconfig_t txconfig_ch9 = {
  0x34,       /* PG delay. */
  0xfefefefe, /* TX power. */
  0x0         /*PG count*/
};


// cfg
void stm32_get_eui64(uint8_t eui[8])
{
    uint32_t uid0 = *(uint32_t *)(UID_BASE + 0x00);
    uint32_t uid1 = *(uint32_t *)(UID_BASE + 0x04);
    uint32_t uid2 = *(uint32_t *)(UID_BASE + 0x08);

    /* DW3000은 보통 little endian EUI 사용 */
    eui[0] = (uid0 >>  0) & 0xFF;
    eui[1] = (uid0 >>  8) & 0xFF;
    eui[2] = (uid0 >> 16) & 0xFF;
    eui[3] = (uid0 >> 24) & 0xFF;
    eui[4] = (uid1 >>  0) & 0xFF;
    eui[5] = (uid1 >>  8) & 0xFF;
    eui[6] = (uid2 >>  0) & 0xFF;
    eui[7] = (uid2 >>  8) & 0xFF;
}

#define UINT16_FMT "%02X%02X"
#define UINT64_FMT "%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X"

#define highByte(x) ((uint8_t)((x) >> 8))
#define lowByte(x)  ((uint8_t)((x) & 0xFF))

typedef struct {
  uint8_t  eui[8];
  uint16_t panId;
  uint16_t addr;
} mac_t;

static mac_t _mac;

uint8_t g_eui[8];
// Legacy mac_t and initialization helpers were removed in Phase 4A in favor of uwb_cfg module.

uwb_error_codes uwb_lib_cfg(dwt_config_t* config)
{
    if (config == NULL) {
        config = &default_config;
    }

    if (dwt_configure(config)) {
        return UWB_ERROR_INIT;
    }

    if (config->chan == 5) {
      dwt_configuretxrf(&txconfig_ch5);
    } else {
      dwt_configuretxrf(&txconfig_ch9);
    }
  
    return UWB_OK;
}
  

/**
 * tag_main_rx
 *  - 
 */
// ===== Constants (Makerfabs example) =====
#ifndef SPEED_OF_LIGHT
#define SPEED_OF_LIGHT 299702547.0
#endif
#ifndef DWT_TIME_UNITS
#define DWT_TIME_UNITS (1.0/499.2e6/128.0)
#endif
#ifndef UUS_TO_DWT_TIME
#define UUS_TO_DWT_TIME 65536ULL
#endif

// ===== Frame layout =====
#define ALL_MSG_COMMON_LEN          10
#define ALL_MSG_SN_IDX               2

// 802.15.4 short addr header offsets
#define POLL_DST_PAN_IDX             3   // 2 bytes
#define POLL_DST_ADDR_IDX            5   // 2 bytes
#define POLL_SRC_ADDR_IDX            7   // 2 bytes
#define POLL_TYPE_IDX                9

#define RESP_MSG_POLL_RX_TS_IDX     10
#define RESP_MSG_RESP_TX_TS_IDX     14
#define ALL_MSG_SN_IDX               2

#define RESP_DST_PAN_IDX             3
#define RESP_DST_ADDR_IDX            5
#define RESP_SRC_ADDR_IDX            7
#define RESP_TYPE_IDX                9
#define RESP_MSG_POLL_RX_TS_IDX     10
#define RESP_MSG_RESP_TX_TS_IDX     14

#ifndef DEFAULT_PAN_ID
#define DEFAULT_PAN_ID UWB_PAN_ID
#endif
#ifndef TAG_SHORT_ADDR
#define TAG_SHORT_ADDR UWB_TAG_SHORT_ADDR
#endif

#ifndef ANCHOR_LEFT_SHORT_ADDR
#define ANCHOR_LEFT_SHORT_ADDR   UWB_ANCHOR_LEFT_SHORT_ADDR
#endif
#ifndef ANCHOR_RIGHT_SHORT_ADDR
#define ANCHOR_RIGHT_SHORT_ADDR  UWB_ANCHOR_RIGHT_SHORT_ADDR
#endif
#ifndef ANCHOR_REAR_SHORT_ADDR
#define ANCHOR_REAR_SHORT_ADDR   UWB_ANCHOR_REAR_SHORT_ADDR
#endif

#ifndef ANCHOR_SHORT_ADDR
#define ANCHOR_SHORT_ADDR UWB_ANCHOR_SHORT_ADDR
#endif

#ifndef NUM_OF_ANCHORS
#define NUM_OF_ANCHORS   UWB_ANCHOR_COUNT
#endif
// Legacy test_anchor_ids and bbx_v2_anchor_list were removed in Phase 4A.
// Ranging logic now uses g_sys_cfg.anchor_ids[].

static uwb_cfg_t g_sys_cfg;

static int8_t twr_anchor_slot_from_id(uint16_t anchor_id)
{
    uint8_t limit = g_sys_cfg.anchor_count;
    if (limit > NUM_OF_ANCHORS) {
        limit = NUM_OF_ANCHORS;
    }

    for (uint8_t i = 0U; i < limit; i++) {
        if (g_sys_cfg.anchor_ids[i] == anchor_id) {
            return (int8_t)i;
        }
    }
    return -1;
}

twr_dist_filter_t* twr_get_filter_by_anchor(uint16_t anchor_id)
{
    int8_t slot = twr_anchor_slot_from_id(anchor_id);
    if (slot < 0) {
        return NULL;
    }
    return &g_twr_dist_filter_by_anchor[(uint8_t)slot];
}

// Legacy cfg_reset_default_ram and cfg_active_* functions have been removed.

// ===== PHY config (from Makerfabs range examples) =====
// static dwt_config_t cfg = {
//   5, DWT_PLEN_128, DWT_PAC8, 9, 9, 1,
//   DWT_BR_6M8, DWT_PHRMODE_STD, DWT_PHRRATE_STD,
//   (129 + 8 - 8),
//   DWT_STS_MODE_OFF, DWT_STS_LEN_64, DWT_PDOA_M0
// };

// // TX power (conservative)
// static dwt_txconfig_t txcfg = { 0xC2, 0x25456585 };

static uint8_t tx_poll_msg[] = {
  0x41, 0x88, 0x00,              // FC, SN
  0x00, 0x00,                    // DEST PAN (patched)
  0x00, 0x00,                    // DEST SHORT ADDR (patched)
  0x00, 0x00,                    // SRC SHORT ADDR (patched)
  0xE0,                          // POLL type
  0x00, 0x00                     // padding
};

// ===== Utils =====
static inline void get_ts4(const uint8_t *src4, uint32_t *lo32) {
  *lo32 = ((uint32_t)src4[3] << 24) | ((uint32_t)src4[2] << 16) |
          ((uint32_t)src4[1] << 8)  |  (uint32_t)src4[0];
}

// Δ1: TWR 통계 구조체 및 전역 변수
typedef struct {
    uint32_t rx_ok;
    uint32_t rx_err;
    uint32_t rx_affrej;
    uint32_t rx_to;
    uint32_t tx_ok;
    uint32_t tx_late;
    uint32_t txfrs_to;
    uint32_t rx2tx_last_cyc;
    uint32_t rx2tx_max_cyc;
    uint32_t last_print_ms;

    // NEW: SW filter counters (PAN/DST mismatch)
    uint32_t rx_fcg_bad_pan;
    uint32_t rx_fcg_bad_dst;
} twr_stats_t;

static twr_stats_t g_tag_stat;
static twr_stats_t g_anc_stat;

/* AFFREJ burst guard */
static uint32_t g_affrej_window_ms = 0U;
static uint32_t g_affrej_window_cnt = 0U;

// Δ1: STAT 로그 출력 (1초 주기) - 구현
static void twr_stats_maybe_print(const char* prefix, twr_stats_t* s)
{
    uint32_t now = HAL_GetTick();
    if ((uint32_t)(now - s->last_print_ms) >= 1000U) {
        s->last_print_ms = now;
        printf("%s[STAT] rx_ok=%lu rx_err=%lu rx_affrej=%lu rx_to=%lu tx_ok=%lu tx_late=%lu txfrs_to=%lu rx2tx_last_cyc=%lu rx2tx_max_cyc=%lu bad_pan=%lu bad_dst=%lu\r\n",
             prefix,
             (unsigned long)s->rx_ok,
             (unsigned long)s->rx_err,
             (unsigned long)s->rx_affrej,
             (unsigned long)s->rx_to,
             (unsigned long)s->tx_ok,
             (unsigned long)s->tx_late,
             (unsigned long)s->txfrs_to,
             (unsigned long)s->rx2tx_last_cyc,
             (unsigned long)s->rx2tx_max_cyc,
             (unsigned long)s->rx_fcg_bad_pan,
             (unsigned long)s->rx_fcg_bad_dst);
    }
}

// ===== State =====
static uint8_t  tag_sn = 0;
static uint8_t  rx_buf[64];

static inline void set_u16_le(uint8_t *p, uint16_t v) {
    p[0] = (uint8_t)(v & 0xFF);
    p[1] = (uint8_t)((v >> 8) & 0xFF);
}

static bool poll_one_aid(uint16_t dst_short, float *out_m) {
    // Clear status
    dwt_forcetrxoff();

    dwt_write32bitreg(SYS_STATUS_ID,
      SYS_STATUS_TXFRS_BIT_MASK |
      SYS_STATUS_RXFCG_BIT_MASK |
      SYS_STATUS_ALL_RX_TO |
      SYS_STATUS_ALL_RX_ERR);

    // RX window arm
    dwt_setrxaftertxdelay(POLL_TX_TO_RESP_RX_DLY_UUS);
    dwt_setrxtimeout(RESP_RX_TIMEOUT_UUS);

    // Compose POLL
    uint8_t this_sn = tag_sn++;
    tx_poll_msg[ALL_MSG_SN_IDX] = this_sn;
    set_u16_le(&tx_poll_msg[POLL_DST_PAN_IDX],  _mac.panId);
    set_u16_le(&tx_poll_msg[POLL_DST_ADDR_IDX], dst_short);
    set_u16_le(&tx_poll_msg[POLL_SRC_ADDR_IDX], _mac.addr);

    dwt_writetxdata(sizeof(tx_poll_msg), tx_poll_msg, 0);
    dwt_writetxfctrl(sizeof(tx_poll_msg) + 2, 0, 1);

    int ret = dwt_starttx(DWT_START_TX_IMMEDIATE | DWT_RESPONSE_EXPECTED);
    if (ret != DWT_SUCCESS) {
        // TX did not start -> avoid falling into RX-only loop
        dwt_forcetrxoff();
        dwt_write32bitreg(SYS_STATUS_ID,
            SYS_STATUS_TXFRS_BIT_MASK |
            SYS_STATUS_RXFCG_BIT_MASK |
            SYS_STATUS_ALL_RX_TO |
            SYS_STATUS_ALL_RX_ERR);

        // [DBG #1] TXFAIL: ret + status
        uint32_t st_now = dwt_read32bitreg(SYS_STATUS_ID);
        printf("[TWR][TAG][TXFAIL] AID=0x%04X SN=%u ret=%d st=0x%08lX\r\n",
             (unsigned int)dst_short,
             (unsigned int)this_sn,
             (int)ret,
             (unsigned long)st_now);
        return false;
    }

    // Save POLL TX timestamp right after TXFRS (MUST observe TXFRS)
    uint32_t poll_tx_ts_saved = 0;
    bool txfrs_ok = false;
    {
      const uint32_t tx_wait_start = micros();
      while (1) {
        uint32_t st_tx = dwt_read32bitreg(SYS_STATUS_ID);

        if (st_tx & SYS_STATUS_TXFRS_BIT_MASK) {
          poll_tx_ts_saved = dwt_readtxtimestamplo32();
          dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_TXFRS_BIT_MASK);
          txfrs_ok = true;
          break;
        }

        // safety timeout to avoid deadlock (e.g. 2ms)
        if ((micros() - tx_wait_start) > 2000U) {
          return false;
        }
      }
    }

    // [PATCH] If TXFRS was not observed, force TRX off and fail immediately.
    // This prevents falling into RX-only loop when TX didn't actually complete.
    if (!txfrs_ok) {
        dwt_forcetrxoff();
        dwt_write32bitreg(SYS_STATUS_ID,
            SYS_STATUS_TXFRS_BIT_MASK |
            SYS_STATUS_RXFCG_BIT_MASK |
            SYS_STATUS_ALL_RX_TO |
            SYS_STATUS_ALL_RX_ERR);

        uint32_t st_now = dwt_read32bitreg(SYS_STATUS_ID);
        printf("[TWR][TAG][TXFRS_TO] AID=0x%04X SN=%u st=0x%08lX\r\n",
             (unsigned int)dst_short,
             (unsigned int)this_sn,
             (unsigned long)st_now);
        return false;
    }

    const uint32_t t_start   = micros();
    const uint32_t window_us = RESP_RX_TIMEOUT_UUS; // NOTE: unit-mix remains for now

    while (true) {
        if ((uint32_t)(micros() - t_start) > 5000U) {
            dwt_forcetrxoff();
            dwt_write32bitreg(SYS_STATUS_ID,
                SYS_STATUS_ALL_RX_ERR | SYS_STATUS_ALL_RX_TO |
                SYS_STATUS_RXFCG_BIT_MASK | SYS_STATUS_TXFRS_BIT_MASK);
            return false;
        }

        uint32_t st;
        do {
            st = dwt_read32bitreg(SYS_STATUS_ID);
            if ((micros() - t_start) >= window_us) {
                st |= SYS_STATUS_ALL_RX_TO;
                break;
            }
        } while (!(st & (SYS_STATUS_RXFCG_BIT_MASK | SYS_STATUS_ALL_RX_TO | SYS_STATUS_ALL_RX_ERR)));

        if (st & SYS_STATUS_RXFCG_BIT_MASK) {
            dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_RXFCG_BIT_MASK);

            uint32_t flen = dwt_read32bitreg(RX_FINFO_ID) & RXFLEN_MASK;
            if (flen > sizeof(rx_buf)) {
                uint32_t elapsed = micros() - t_start;
                if (elapsed >= window_us) return false;
                dwt_setrxtimeout(window_us - elapsed);
                dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_ALL_RX_ERR | SYS_STATUS_ALL_RX_TO);
                dwt_rxenable(DWT_START_RX_IMMEDIATE);
                continue;
            }

            dwt_readrxdata(rx_buf, flen, 0);

            // RESP 수신 후
            uint8_t resp_sn = rx_buf[ALL_MSG_SN_IDX];

            // memcmp 기반 hdr_ok 제거 (포맷 전환 중 깨지기 쉬움)
            // 최소 필드 검증으로 대체: FC + RESP type
            bool fc_ok = (flen > 1 && rx_buf[0] == 0x41 && rx_buf[1] == 0x88);
            bool type_ok = (flen > RESP_TYPE_IDX && rx_buf[RESP_TYPE_IDX] == 0xE1);

            uint16_t pan_rx = 0xFFFF, dst_rx = 0xFFFF, src_rx = 0xFFFF;
            if (flen > (RESP_DST_PAN_IDX + 1))
                pan_rx = (uint16_t)rx_buf[RESP_DST_PAN_IDX] | ((uint16_t)rx_buf[RESP_DST_PAN_IDX + 1] << 8);
            if (flen > (RESP_DST_ADDR_IDX + 1))
                dst_rx = (uint16_t)rx_buf[RESP_DST_ADDR_IDX] | ((uint16_t)rx_buf[RESP_DST_ADDR_IDX + 1] << 8);
            if (flen > (RESP_SRC_ADDR_IDX + 1))
                src_rx = (uint16_t)rx_buf[RESP_SRC_ADDR_IDX] | ((uint16_t)rx_buf[RESP_SRC_ADDR_IDX + 1] << 8);

            bool hdr_ok = fc_ok && type_ok;
            bool addr_ok = (pan_rx == _mac.panId) && (dst_rx == _mac.addr) && (src_rx == dst_short);

            if (!(hdr_ok && resp_sn == this_sn && addr_ok)) {
                uint32_t elapsed = micros() - t_start;
                if (elapsed >= window_us) return false;
                dwt_setrxtimeout(window_us - elapsed);
                dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_ALL_RX_ERR | SYS_STATUS_ALL_RX_TO);
                dwt_rxenable(DWT_START_RX_IMMEDIATE);
                continue;
            }

            // === SS-TWR distance only ===
            uint32_t poll_tx_ts = (poll_tx_ts_saved != 0U) ? poll_tx_ts_saved : dwt_readtxtimestamplo32();
            uint32_t resp_rx_ts = dwt_readrxtimestamplo32();
            uint32_t poll_rx_ts, resp_tx_ts;

            float clockOffsetRatio = ((float)dwt_readclockoffset()) / (uint32_t)(1 << 26);

            get_ts4(&rx_buf[RESP_MSG_POLL_RX_TS_IDX], &poll_rx_ts);
            get_ts4(&rx_buf[RESP_MSG_RESP_TX_TS_IDX], &resp_tx_ts);

            int32_t rtd_init = (int32_t)(resp_rx_ts - poll_tx_ts);
            int32_t rtd_resp = (int32_t)(resp_tx_ts - poll_rx_ts);

            double tof  = ((rtd_init - (double)rtd_resp * (1.0 - (double)clockOffsetRatio)) / 2.0) * DWT_TIME_UNITS;
            double dist = tof * SPEED_OF_LIGHT;
            if (dist < 0.0) dist = 0.0;

            *out_m = (float)dist;

            long dist_mm = 0L;
            if (dist > 0.0) {
                double dist_mm_d = dist * 1000.0;   // m → mm
                if (dist_mm_d > 2147483647.0) {
                    dist_mm_d = 2147483647.0;
                }
                dist_mm = (int32_t)dist_mm_d;
            }

            // Δ2: dist_mm 계산 완료 후, 즉시 diagnostics 읽기
            dwt_rxdiag_t diag;
            memset(&diag, 0, sizeof(diag));
            dwt_readdiagnostics(&diag);
            // --- RXDIAG RAW log (for host-side visualization) ---
            // NOTE: keep 32-bit safe prints. xtalOffset uses signed 16-bit from dwt_readclockoffset().
            {
                int16_t xtal_off = (int16_t)dwt_readclockoffset();
                printf("[DW3][RXDIAG][RAW] ipatovPeak=%lu ipatovPower=%lu ipatovF1=%lu ipatovFpIndex=%lu ipatovAccumCount=%lu xtalOffset=%ld\r\n",
                       (unsigned long)diag.ipatovPeak,
                       (unsigned long)diag.ipatovPower,
                       (unsigned long)diag.ipatovF1,
                       (unsigned long)diag.ipatovFpIndex,
                       (unsigned long)diag.ipatovAccumCount,
                       (long)xtal_off);
            }


            // Δ2: DIAG sanity check
            bool diag_ok = twr_diag_is_sane(&diag);

            // Δ2: 품질 점수 계산
            twr_quality_t q = twr_quality_from_diag(&diag);


            // Same-frame diagnostics consistency check:
// - Our logged q.noise_raw is (ipatovF1 & 0x3FFFFF) clamped to 16b
// - Our logged q.cir_pwr_raw is (ipatovPower & 0x1FFFF) clamped to 16b
            {
                // Same-frame diagnostics consistency check (sanity): raw fields should be non-zero and stable.
                if ((diag.ipatovF1 == 0U) || ((diag.ipatovPower & 0x1FFFFUL) == 0U) || ((diag.ipatovPeak & 0x1FFFFFUL) == 0U)) {
                    printf("[TWR][DIAGCHK] AID=0x%04X SN=%u suspicious F1=%lu PWR=%lu PEAK=%lu\r\n",
                           (unsigned int)dst_short, (unsigned int)this_sn,
                           (unsigned long)diag.ipatovF1, (unsigned long)diag.ipatovPower, (unsigned long)diag.ipatovPeak);
                }

                // If our extracted/clamped values don't match masked raw values, report once (helps catch domain mistakes).
                if ((uint32_t)q.noise_raw != (uint32_t)(diag.ipatovF1 & 0x3FFFFFUL) ||
                    (uint32_t)q.cir_pwr_raw != (uint32_t)(diag.ipatovPower & 0x1FFFFUL)) {
                    printf("[TWR][DIAGCHK] AID=0x%04X SN=%u mismatch noise_raw=%u(F1=%lu) cir_raw=%u(PWR=%lu)\r\n",
                           (unsigned int)dst_short, (unsigned int)this_sn,
                           (unsigned int)q.noise_raw, (unsigned long)diag.ipatovF1,
                           (unsigned int)q.cir_pwr_raw, (unsigned long)diag.ipatovPower);
                }
            }
// FP gap 게이트 계산 (diag_ok일 때만)
            bool fp_ok = true;
            uint32_t fp_gap_abs_q6_u32 = 0;
            int32_t fp_gap_q6_s = 0;

            if (diag_ok) {
                int32_t fp_peak_idx_q6_s = ((int32_t)q.peak_idx) << 6;
                int32_t fp_fp_idx_q6_s   = (int32_t)q.fp_idx_q6;
                fp_gap_q6_s = fp_peak_idx_q6_s - fp_fp_idx_q6_s;

                int32_t fp_gap_abs_q6_s = (fp_gap_q6_s < 0) ? -fp_gap_q6_s : fp_gap_q6_s;
                if (fp_gap_abs_q6_s < 0) fp_gap_abs_q6_s = 0;
                fp_gap_abs_q6_u32 = (uint32_t)fp_gap_abs_q6_s;

                uint32_t gap_samp_u32 = (fp_gap_abs_q6_u32 + 32U) >> 6;  // q6 -> sample(round)

                if (gap_samp_u32 > (uint32_t)FP_GAP_MAX_SAMP) {
                    fp_ok = false;
                } else {
                    fp_ok = true;
                }

                printf("[TWR][FPCHK] AID=0x%04X SN=%u gap_q6=%lu gap_samp=%lu th_samp=%lu fpok=%u\r\n",
                       (unsigned int)dst_short,
                       (unsigned int)this_sn,
                       (unsigned long)fp_gap_abs_q6_u32,
                       (unsigned long)gap_samp_u32,
                       (unsigned long)FP_GAP_MAX_SAMP,
                       (unsigned int)(fp_ok ? 1U : 0U));

                // --- TAG FP log (nlos + indices) for host-side visualization ---
                // nlos_score is a heuristic (0~100). gap_q6 can be signed; here we log signed gap_q6_s as %ld.
                printf("[TWR][TAG][FP] AID=0x%04X SN=%u fp_idx_q6=%lu peak_idx_q6=%lu gap_q6=%ld nlos=%u\r\n",
                       (unsigned int)dst_short,
                       (unsigned int)this_sn,
                       (unsigned long)q.fp_idx_q6,
                       (unsigned long)(((uint32_t)q.peak_idx) << 6),
                       (long)fp_gap_q6_s,
                       (unsigned int)q.nlos_score);

            }

            // gate 계산에 필요한 정수값 구성 (앵커별 필터 상태 참조)
            twr_dist_filter_t *af = twr_get_filter_by_anchor(dst_short);

            long dist_raw_mm_l = (long)(dist_mm + 0.5f);
            long iir_mm_l = -1L;
            uint8_t filt_samples = 0U;

            if (af != NULL) {
                if (af->iir_initialized != 0U) {
                    iir_mm_l = (long)(af->iir_value + 0.5f);
                }
                filt_samples = af->count;
            }

            // gap_q6를 uint16_t로 변환 (절댓값) - q_weight 입력용
            uint16_t gap_q6_u16 = (uint16_t)((fp_gap_abs_q6_u32 > 65535U) ? 65535U : fp_gap_abs_q6_u32);

            const char *qw_reason = NULL;
            uint8_t q_weight = twr_quality_weight_u8(
                dist_raw_mm_l,
                (uint16_t)q.rssi_raw,
                (uint8_t)q.snr_score,
                q.fp_amp_raw,
                gap_q6_u16,
                iir_mm_l,
                filt_samples,
                &qw_reason
            );

                        // -----------------------------------------------------------------
            // Quality gating policy (FINAL)
            //
            // ✅ Do NOT fail solely due to low RSSI.
            //    - RSSI is treated as a *warning* only (logged in reason).
            // ✅ Hard fail conditions:
            //    - diag_ok == true
            //    - fp_ok   == true
            //    - snr_score >= TWR_QUAL_MIN_SNR
            //    - mp_score  >= TWR_QUAL_MIN_MP
            // -----------------------------------------------------------------

            const bool warn_rssi_low = (q.rssi_score < TWR_QUAL_WARN_RSSI);
            const bool fail_diag = (diag_ok == false);
            const bool fail_fp   = (fp_ok == false);
            const bool fail_snr  = (q.snr_score < TWR_QUAL_MIN_SNR);
            const bool fail_mp   = (q.mp_score  < TWR_QUAL_MIN_MP);

            bool quality_ok = (fail_diag == false) && (fail_fp == false) && (fail_snr == false) && (fail_mp == false);

            // Detailed reason string for logs (ok + warnings, or fail tokens).
            char reason_str[64];
            reason_str[0] = '\0';

            if (quality_ok) {
                twr_reason_add(reason_str, sizeof(reason_str), "ok");
                if (warn_rssi_low) {
                    twr_reason_add(reason_str, sizeof(reason_str), "warn_rssi_low");
                }
            } else {
                if (fail_diag) { twr_reason_add(reason_str, sizeof(reason_str), "diag"); }
                if (fail_fp)   { twr_reason_add(reason_str, sizeof(reason_str), "fp"); }
                if (fail_snr)  { twr_reason_add(reason_str, sizeof(reason_str), "snr"); }
                if (fail_mp)   { twr_reason_add(reason_str, sizeof(reason_str), "mp"); }
                if (warn_rssi_low) { twr_reason_add(reason_str, sizeof(reason_str), "rssi_low"); }
            }

            const char* quality_fail_reason = NULL;
            if (!quality_ok) {
                quality_fail_reason = reason_str;
            }

            printf("[TWR][TAG][QUAL] AID=0x%04X SN=%u rssi=%u mp=%u snr=%u ok=%u fpok=%u reason=%s (rssi_raw=%u fp=%u cir=%u noise=%u)\r\n",
                   (unsigned int)dst_short,
                   (unsigned int)this_sn,
                   (unsigned int)q.rssi_score,
                   (unsigned int)q.mp_score,
                   (unsigned int)q.snr_score,
                   (unsigned int)(quality_ok ? 1U : 0U),
                   (unsigned int)(fp_ok ? 1U : 0U),
                   reason_str,
                   (unsigned int)q.rssi_raw,
                   (unsigned int)q.fp_amp_raw,
                   (unsigned int)q.cir_pwr_raw,
                   (unsigned int)q.noise_raw);

            // Distance filter update + ground-truth stats (if enabled).
            twr_dist_filter_update_and_log_mm_ex((uint16_t)dst_short, this_sn, (float)(dist * 1000.0),
                                                quality_ok, q_weight, quality_fail_reason);
        return true;
        } else {
            printf("[RX FAIL] st=0x%08lX\r\n", (unsigned long)st);

            // 기존처럼 클리어
            dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_ALL_RX_ERR | SYS_STATUS_ALL_RX_TO);

        }

        // RX_TO / RX_ERR handling
        {
            uint32_t elapsed = micros() - t_start;
            if (elapsed < window_us) {
                uint32_t remain = window_us - elapsed;
                dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_ALL_RX_ERR | SYS_STATUS_ALL_RX_TO);
                dwt_setrxtimeout(remain);
                dwt_rxenable(DWT_START_RX_IMMEDIATE);
                continue;
            }

            dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_ALL_RX_TO | SYS_STATUS_ALL_RX_ERR);
            return false;
        }
    }
}

static void dw_recover_idle(void)
{
    // Strong recovery: stop any ongoing RX/TX
    dwt_forcetrxoff();

    // Clear pending status (RX/TX)
    dwt_write32bitreg(SYS_STATUS_ID,
        SYS_STATUS_TXFRS_BIT_MASK |
        SYS_STATUS_RXFCG_BIT_MASK |
        SYS_STATUS_ALL_RX_TO |
        SYS_STATUS_ALL_RX_ERR);

    // small settle (keep short)
    HAL_Delay(1);
}

static void tag_print_multi_anchor_summary(uint8_t n, const bool ok[], const float dist_m[])
{
    uint8_t ok_cnt = 0U;

    for (uint8_t i = 0U; i < n; i++) {
        if (ok[i]) ok_cnt++;
    }

    printf("[TAG][MULTI] ok=%u/%u",
           (unsigned int)ok_cnt,
           (unsigned int)n);

    for (uint8_t i = 0U; i < n; i++) {
        uint16_t aid = g_sys_cfg.anchor_ids[i];

        if (aid == 0U) {
            printf(" A%u=0x0000:SKIP", (unsigned int)i);
            continue;
        }

        if (!ok[i]) {
            printf(" A%u=0x%04X:FAIL",
                   (unsigned int)i,
                   (unsigned int)aid);
            continue;
        }

        long dist_mm_l = 0L;
        if (dist_m[i] > 0.0f) {
            float mm_f = dist_m[i] * 1000.0f;
            if (mm_f > 2147483000.0f) mm_f = 2147483000.0f;
            dist_mm_l = (long)(mm_f + 0.5f);
        }

        printf(" A%u=0x%04X:%ldmm",
               (unsigned int)i,
               (unsigned int)aid,
               dist_mm_l);
    }

    printf("\r\n");
}

void tag_main(void) {
    while (1)
    {
        /* NOTE:
         * - anchor_count/anchor_ids는 cfg_reset_default_ram() 또는 Flash 로드 결과를 사용함.
         * - NUM_OF_ANCHORS(고정 상수)로 돌리면, anchor_ids 미설정(0x0000) 대상까지 POLL을 보내서
         *   ANCHOR 쪽에서 AFFREJ가 증가하고 TAG 쪽은 RX FAIL/timeout이 난다.
         */
        uint8_t n = g_sys_cfg.anchor_count;
        if (n == 0U) n = 1U;
        if (n > 8U)  n = 8U;

        bool  ok[8];
        float dist_m[8];

        for (uint8_t i = 0; i < n; i++) {
            ok[i]     = false;
            dist_m[i] = 0.0f;
        }

        /* 1st try */
        for (uint8_t i = 0; i < n; i++) {
            uint16_t aid = g_sys_cfg.anchor_ids[i];
            if (aid == 0U) {
                ok[i] = false;
                continue;
            }
            ok[i] = poll_one_aid(aid, &dist_m[i]);
        }

        /* retry only failures */
        for (uint8_t i = 0; i < n; i++) {
            if (ok[i]) continue;
            uint16_t aid = g_sys_cfg.anchor_ids[i];
            if (aid == 0U) continue;
            
            dw_recover_idle();
            ok[i] = poll_one_aid(aid, &dist_m[i]);
        }

        /* NEW: one-line summary for multi-anchor ranging */
        tag_print_multi_anchor_summary(n, ok, dist_m);

        /* stats aggregation */
        for (uint8_t i = 0; i < n; i++) {
            if (ok[i]) g_tag_stat.rx_ok++;
            else       g_tag_stat.rx_err++;
        }

        /* tight loop guard / backoff */
        {
            uint16_t backoff_us = 800U + (uint16_t)(rand() & 0x07FFU); /* 0.8~2.847 ms */
            micros_delay(backoff_us);
        }
    }
}



// ANCHOR

#ifndef UUS_TO_DWT_TIME
#define UUS_TO_DWT_TIME 65536ULL
#endif

// Frame indexes
// #define ALL_MSG_COMMON_LEN        10
// #define ALL_MSG_SN_IDX             2
// #define POLL_DEST_AID_IDX         12
// #define RESP_MSG_POLL_RX_TS_IDX   10
// #define RESP_MSG_RESP_TX_TS_IDX   14
// #define RESP_SRC_AID_IDX          18

// PHY config (same as example)
// static dwt_config_t cfg = {
//   5, DWT_PLEN_128, DWT_PAC8, 9, 9, 1,
//   DWT_BR_6M8, DWT_PHRMODE_STD, DWT_PHRRATE_STD,
//   (129 + 8 - 8),
//   DWT_STS_MODE_OFF, DWT_STS_LEN_64, DWT_PDOA_M0
// };
// static dwt_txconfig_t txcfg = { 0xC2, 0x25456585 };

// RESP: [FCF2][SN][DST_PAN2][DST_ADDR2][SRC_ADDR2][TYPE][TS1_4][TS2_4][RSV]
static uint8_t tx_resp_msg[] = {
  0x41, 0x88, 0x00,   // 0..2
  0x00, 0x00,         // 3..4  DST PAN
  0x00, 0x00,         // 5..6  DST short (tag)
  0x00, 0x00,         // 7..8  SRC short (anchor)
  0xE1,               // 9     RESP type
  0,0,0,0,            // 10..13 TS1 poll_rx
  0,0,0,0,            // 14..17 TS2 resp_tx
  0x00                // 18     reserved
};


// Utils
static inline void put_ts4(uint8_t *dst4, uint64_t ts) {
  dst4[0] = (uint8_t) ts;
  dst4[1] = (uint8_t)(ts >> 8);
  dst4[2] = (uint8_t)(ts >> 16);
  dst4[3] = (uint8_t)(ts >> 24);
}
static inline void hexdump(const char* tag, const uint8_t* p, uint32_t n)
{
    // 필요에 따라 사이즈 조정 (너무 크면 스택 말고 static으로 빼도 됨)
    char buf[256];
    int pos = 0;

    // 헤더
    pos += snprintf(&buf[pos], sizeof(buf) - (size_t)pos,
                    "%s (%lu bytes): ", tag, (unsigned long)n);

    // 본문
    for (uint32_t i = 0; i < n && pos < (int)sizeof(buf) - 4; i++) {
        pos += snprintf(&buf[pos], sizeof(buf) - (size_t)pos,
                        "%02X ", (unsigned int)p[i]);
    }

    // 줄바꿈
    if (pos < (int)sizeof(buf) - 2) {
        buf[pos++] = '\r';
        buf[pos++] = '\n';
        buf[pos]   = '\0';
    } else {
        buf[sizeof(buf) - 3] = '\r';
        buf[sizeof(buf) - 2] = '\n';
        buf[sizeof(buf) - 1] = '\0';
    }

    printf("%s", buf);
}

static inline uint16_t get_u16_le(const uint8_t *p) {
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

void anchor_main()
{
    printf("[RX][APP] Starting RX application\r\n");

    /* --- 루프에서 재사용할 상수/오프셋 미리 계산 (DTU/H 단위) --- */
    const uint32_t resp_tx_dly_dtu_h =
        (uint32_t)(((uint64_t)POLL_RX_TO_RESP_TX_DLY_UUS * UUS_TO_DWT_TIME) >> 8);

    const uint16_t resp_tx_antd_offset = (uint16_t)(TX_ANT_DLY + RESP_TS_ADJ_UNITS);

    while (1)
    {
        /* --- RX 준비: 상태 클리어 + 즉시 RX 시작 --- */
        dwt_write32bitreg(SYS_STATUS_ID,
                          SYS_STATUS_RXFCG_BIT_MASK |
                          SYS_STATUS_ALL_RX_ERR     |
                          SYS_STATUS_ALL_RX_TO      |
                          SYS_STATUS_TXFRS_BIT_MASK);

        dwt_rxenable(DWT_START_RX_IMMEDIATE);

        /* RX 완료 or 에러 or 타임아웃 대기 */
        uint32_t status;
        do
        {
            status = dwt_read32bitreg(SYS_STATUS_ID);
        } while (!(status & (SYS_STATUS_RXFCG_BIT_MASK | SYS_STATUS_AFFREJ_BIT_MASK | SYS_STATUS_RXERR_ALL_MASK | SYS_STATUS_RXTO_ALL_MASK)));

        /* RX 에러/타임아웃/AFFREJ 경로 */
        if ((status & SYS_STATUS_RXFCG_BIT_MASK) == 0U)
        {
            uint32_t clr = 0U;
            /* const uint32_t fce    = ((status & SYS_STATUS_RXFCE_BIT_MASK) != 0U) ? 1U : 0U;
            const uint32_t phe    = ((status & SYS_STATUS_RXPHE_BIT_MASK) != 0U) ? 1U : 0U;
            const uint32_t rfsl   = ((status & SYS_STATUS_RXFSL_BIT_MASK) != 0U) ? 1U : 0U;
            const uint32_t rfto   = ((status & SYS_STATUS_RXFTO_BIT_MASK) != 0U) ? 1U : 0U; */
            const uint32_t arfe   = ((status & SYS_STATUS_ARFE_BIT_MASK) != 0U) ? 1U : 0U;
            const uint32_t affrej = ((status & SYS_STATUS_AFFREJ_BIT_MASK) != 0U) ? 1U : 0U;
            const uint32_t is_affrej = (affrej || arfe) ? 1U : 0U;
            const uint32_t is_to     = ((status & SYS_STATUS_RXTO_ALL_MASK)  != 0U) ? 1U : 0U;
            // const uint32_t is_err    = ((status & SYS_STATUS_RXERR_ALL_MASK) != 0U) ? 1U : 0U;

            /* ARFE(=주소/PAN 필터 reject)는 rx_err에서 제외해서 통계 분리 */
             const uint32_t rxerr_no_arfe = (SYS_STATUS_RXERR_ALL_MASK & ~SYS_STATUS_ARFE_BIT_MASK);
             const uint32_t is_err    = ((status & rxerr_no_arfe) != 0U) ? 1U : 0U;

            /* 분리 집계: AFFREJ는 rx_err와 분리 */
            if (is_affrej) { g_anc_stat.rx_affrej++; clr |= SYS_STATUS_AFFREJ_BIT_MASK; }
            if (is_to)     { g_anc_stat.rx_to++;     clr |= SYS_STATUS_RXTO_ALL_MASK;  }
            if ((!is_affrej) && (!is_to) && is_err) { g_anc_stat.rx_err++; clr |= SYS_STATUS_RXERR_ALL_MASK; }
            if (is_affrej && is_err) { clr |= SYS_STATUS_RXERR_ALL_MASK; } /* 상태 비트는 클리어 */

            /* AFFREJ burst guard: short window burst -> RX re-arm */
            {
                uint32_t now_ms = HAL_GetTick();
                if ((uint32_t)(now_ms - g_affrej_window_ms) > 50U) {
                    g_affrej_window_ms = now_ms;
                    g_affrej_window_cnt = 0U;
                }
                if (is_affrej) g_affrej_window_cnt++;
                if (g_affrej_window_cnt >= 12U) {
                    dwt_forcetrxoff();
                    dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_ALL_RX_ERR | SYS_STATUS_ALL_RX_TO | SYS_STATUS_AFFREJ_BIT_MASK);
                    dwt_rxenable(DWT_START_RX_IMMEDIATE);
                    g_affrej_window_cnt = 0U;
                }
            }

            if (clr != 0U) dwt_write32bitreg(SYS_STATUS_ID, clr);

            /* RXST rate limit: AFFREJ 64건당 1회 */
            /* if ((!is_affrej) || ((g_anc_stat.rx_affrej & 0x3FUL) == 0UL)) {
                printf("[RXST] st=0x%08lX fcg=0 affrej=%lu fce=%lu phe=%lu rfsl=%lu rfto=%lu arfe=%lu\r\n",
                       (unsigned long)status,
                       (unsigned long)affrej,
                       (unsigned long)fce,
                       (unsigned long)phe,
                       (unsigned long)rfsl,
                       (unsigned long)rfto,
                       (unsigned long)arfe);
            } */

            twr_stats_maybe_print("[ANC]", &g_anc_stat);
            continue;
        }

        /* 정상 수신(RXFCG) */
        dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_RXFCG_BIT_MASK);

        /* 프레임 길이 확인 */
        uint32_t flen = dwt_read32bitreg(RX_FINFO_ID) & RXFLEN_MASK;
        if (flen > sizeof(rx_buf) || flen < ALL_MSG_COMMON_LEN)
        {
            continue;
        }

        dwt_readrxdata(rx_buf, flen, 0);

        /* 공통 헤더/필드 */
        const uint8_t  rx_sn = rx_buf[ALL_MSG_SN_IDX];

        const bool fc_ok   = (rx_buf[0] == 0x41 && rx_buf[1] == 0x88);
        const bool type_ok = (flen > POLL_TYPE_IDX) && (rx_buf[POLL_TYPE_IDX] == 0xE0);

        if (!fc_ok || !type_ok) continue;

        /* 주소 필드는 길이 보장 후 파싱 */
        if (flen <= (POLL_DST_PAN_IDX + 1) ||
            flen <= (POLL_DST_ADDR_IDX + 1) ||
            flen <= (POLL_SRC_ADDR_IDX + 1))
        {
            continue;
        }

        const uint16_t rx_dst_pan   = get_u16_le(&rx_buf[POLL_DST_PAN_IDX]);
        const uint16_t rx_dst_addr  = get_u16_le(&rx_buf[POLL_DST_ADDR_IDX]);
        const uint16_t poll_src_addr = get_u16_le(&rx_buf[POLL_SRC_ADDR_IDX]); /* 중요: RESP DST로 사용 */

        /* SW 필터(하드웨어 FF와 별개로 안전하게 재확인) */
         /* 검증용: HW FF를 통과해 RXFCG까지 온 "불일치" 프레임 카운트 */
        if (rx_dst_pan != _mac.panId) { g_anc_stat.rx_fcg_bad_pan++; twr_stats_maybe_print("[ANC]", &g_anc_stat); continue; }
        if (rx_dst_addr != _mac.addr && rx_dst_addr != 0xFFFF) { g_anc_stat.rx_fcg_bad_dst++; twr_stats_maybe_print("[ANC]", &g_anc_stat); continue; }

        g_anc_stat.rx_ok++;

        /* === 크리티컬 파트 === */

        /* 1) POLL RX timestamp */
        const uint64_t poll_rx_ts_40 = get_rx_timestamp_u64();
        const uint32_t poll_rx_ts_h  = (uint32_t)((poll_rx_ts_40 & 0xFFFFFFFFFFULL) >> 8);

        /* 2) RESP 지연 TX 목표 시각 */
        const uint32_t target_h = poll_rx_ts_h + resp_tx_dly_dtu_h;
        dwt_setdelayedtrxtime(target_h);

        const uint64_t resp_tx_ts_40 =
            (((uint64_t)(target_h & 0xFFFFFFFEUL)) << 8) +
            (uint64_t)resp_tx_antd_offset;

        /* 3) RESP 프레임 구성 */
        tx_resp_msg[ALL_MSG_SN_IDX] = rx_sn;
        tx_resp_msg[RESP_TYPE_IDX]  = 0xE1;

        set_u16_le(&tx_resp_msg[RESP_DST_PAN_IDX],  _mac.panId);
        set_u16_le(&tx_resp_msg[RESP_DST_ADDR_IDX], poll_src_addr); /* POLL 송신자에게 응답 */
        set_u16_le(&tx_resp_msg[RESP_SRC_ADDR_IDX], _mac.addr);

        put_ts4(&tx_resp_msg[RESP_MSG_POLL_RX_TS_IDX],  poll_rx_ts_40);
        put_ts4(&tx_resp_msg[RESP_MSG_RESP_TX_TS_IDX], resp_tx_ts_40);

        /* 4) TX 시작 */
        dwt_writetxdata(sizeof(tx_resp_msg), tx_resp_msg, 0);
        dwt_writetxfctrl(sizeof(tx_resp_msg) + 2, 0, 1);

        const int tx_ret = dwt_starttx(DWT_START_TX_DELAYED);

        if (tx_ret == DWT_ERROR)
        {
            const uint32_t now_h  = dwt_readsystimestamphi32();
            const uint64_t now_40 = ((uint64_t)now_h) << 8;

            int32_t late_us = 0;
            if (now_40 > resp_tx_ts_40)
            {
                uint64_t diff = now_40 - resp_tx_ts_40;
                if (diff > 0x7FFFFFFFULL) diff = 0x7FFFFFFFULL;
                late_us = (int32_t)(diff / UUS_TO_DWT_TIME);
            }

            g_anc_stat.tx_late++;
            twr_stats_maybe_print("[ANC]", &g_anc_stat);
            printf("[A][LATE] SN=%u (~%ld us late)\r\n", (unsigned)rx_sn, (long)late_us);
            continue;
        }

        /* TX 완료 대기 */
        uint32_t st_tx;
        do
        {
            st_tx = dwt_read32bitreg(SYS_STATUS_ID);
        } while ((st_tx & SYS_STATUS_TXFRS_BIT_MASK) == 0U);

        dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_TXFRS_BIT_MASK);

        g_anc_stat.tx_ok++;
        printf("[A] RESP TX done (SN=%u)\r\n", (unsigned)rx_sn);
    }
}



static void uwb_print_radio_params_3lines(void)
{
    static uint16_t g_tx_antd_set = (uint16_t)TX_ANT_DLY;
    static uint16_t g_rx_antd_set = (uint16_t)RX_ANT_DLY;
    // (1) ANTD: readback (실제 적용값)
    {
        printf("[UWB][ANTD] tx_ant_dly=%u rx_ant_dly=%u\r\n",
               (unsigned)g_tx_antd_set, (unsigned)g_rx_antd_set);
    }

    // (2) CFG: 지금 코드 구조상 uwb_lib_cfg(NULL) -> default_config가 적용됨
    {
        const dwt_config_t *c = &default_config;
        printf("[UWB][CFG] chan=%u plen=%u pac=%u txCode=%u rxCode=%u sfdType=%u dr=%u phrMode=%u phrRate=%u stsMode=%u stsLen=%u pdoa=%u sfdTO=%u\r\n",
               (unsigned)c->chan,
               (unsigned)c->txPreambLength,
               (unsigned)c->rxPAC,
               (unsigned)c->txCode,
               (unsigned)c->rxCode,
               (unsigned)c->sfdType,
               (unsigned)c->dataRate,
               (unsigned)c->phrMode,
               (unsigned)c->phrRate,
               (unsigned)c->stsMode,
               (unsigned)c->stsLength,
               (unsigned)c->pdoaMode,
               (unsigned)c->sfdTO);
    }

    // (3) CLK: 32-bit safe ppm 근사 (64-bit/LL 금지)
    {
        int16_t clk_off = (int16_t)dwt_readclockoffset();

        // ppm ≈ clk_off * (1e6 / 2^26) = clk_off * (15625 / 2^20)
        // 최대값: 32767 * 15625 = 511,984,375 (32-bit signed 안전)
        int32_t prod = (int32_t)clk_off * (int32_t)15625;
        // rounding: +/- 0.5 LSB
        if (prod >= 0) prod += (1 << 19);
        else           prod -= (1 << 19);
        int32_t ppm = (prod >> 20);

        printf("[UWB][CLK] clk_off=%ld ppm=%ld\r\n", (long)clk_off, (long)ppm);
    }
}



static void apply_uwb_cfg(uwb_cfg_t *cfg)
{
    // Step 1: Radio PHY config — restore known-good uwb_lib_cfg(NULL) path
    // This calls dwt_configure(&default_config) + dwt_configuretxrf(&txconfig_ch5)
    uwb_lib_cfg(NULL);

    // Step 2: MAC config — from known-good set_uwb_cfg()
    stm32_get_eui64(g_eui);
    dwt_seteui(g_eui);
    dwt_setpanid(cfg->pan_id);
    dwt_setaddress16(cfg->short_addr);
    memcpy(_mac.eui, g_eui, 8);
    _mac.panId = cfg->pan_id;
    _mac.addr  = cfg->short_addr;

    // Step 3: Frame filter + LEDs (identical in both versions)
    dwt_configureframefilter(DWT_FF_ENABLE_802_15_4, DWT_FF_DATA_EN | DWT_FF_ACK_EN);
    dwt_setleds(DWT_LEDS_ENABLE | DWT_LEDS_INIT_BLINK);

    // Step 4: Antenna delays (present in 126faf3, was missing in HEAD)
    dwt_setrxantennadelay(RX_ANT_DLY);
    dwt_settxantennadelay(TX_ANT_DLY);

    // Step 5: RF amplifiers (present in 126faf3, was missing in HEAD)
    dwt_setlnapamode(DWT_LNA_ENABLE | DWT_PA_ENABLE);

    // Step 6: CIA diagnostics (present in 126faf3, was missing in HEAD)
    dwt_configciadiag(DW_CIA_DIAG_LOG_ALL);

    // Step 7: Sleep config (present in 126faf3, was missing in HEAD)
    dwt_configuresleep(DWT_CONFIG | DWT_LOADLDO | DWT_LOADDGC | DWT_LOADBIAS | DWT_GOTORX,
                       DWT_WAKE_CSN | DWT_SLEEP | DWT_SLP_EN | DWT_PRES_SLEEP);
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_SPI1_Init();
  MX_LPUART1_UART_Init();
  MX_FDCAN1_Init();
  MX_UART5_Init();
  MX_FMAC_Init();
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */
  micros_init();

#ifdef UWB_ROLE_TX
  uint8_t compiled_role = UWB_ROLE_TAG;
#else
  uint8_t compiled_role = UWB_ROLE_ANCHOR;
#endif

  uint8_t runtime_role;

  // 1) Initialize configuration from Flash Persistence (Phase 4A)
  if (!uwb_cfg_load(&g_sys_cfg)) {
      printf("\r\n[CFG] Flash load failed. Falling back to compiled role...\r\n");
      uwb_cfg_get_default(&g_sys_cfg, compiled_role);
      runtime_role = compiled_role;
  } else if (g_sys_cfg.role != UWB_ROLE_TAG && g_sys_cfg.role != UWB_ROLE_ANCHOR) {
      printf("\r\n[CFG] Invalid role (0x%02X) in Flash. Falling back to compiled role...\r\n", g_sys_cfg.role);
      runtime_role = compiled_role;
  } else {
      printf("\r\n[CFG] Flash config loaded successfully. Role: %s\r\n", 
             (g_sys_cfg.role == UWB_ROLE_TAG) ? "TAG" : "ANCHOR");
      runtime_role = g_sys_cfg.role;
  }

// Test harness: enable via CMake (-DUWB_TEST_ROLE_SWITCH) or uncomment below
// #define UWB_TEST_ROLE_SWITCH
#ifdef UWB_TEST_ROLE_SWITCH
  if (g_sys_cfg.pan_id == UWB_DEFAULT_PAN_ID && g_sys_cfg.role == compiled_role) {
      printf("[TEST-RS] Step 1: Default Role (%s) detected.\r\n", (compiled_role == UWB_ROLE_TAG) ? "TAG" : "ANCHOR");
      printf("[TEST-RS] Step 2: Switching to opposite role (%s) and saving...\r\n", 
             (compiled_role == UWB_ROLE_TAG) ? "ANCHOR" : "TAG");
      g_sys_cfg.role = (compiled_role == UWB_ROLE_TAG) ? UWB_ROLE_ANCHOR : UWB_ROLE_TAG;
      g_sys_cfg.pan_id = 0xC0DE; 
      uwb_cfg_update_crc(&g_sys_cfg);
      if (uwb_cfg_save(&g_sys_cfg)) {
          printf("[TEST-RS] Step 3: Save SUCCESS. Reboot to verify ROLE SWITCH.\r\n");
      }
  } else if (g_sys_cfg.pan_id == 0xC0DE) {
      printf("[TEST-RS] VERIFICATION: Loaded Role = %s (Compiled = %s)\r\n",
             (g_sys_cfg.role == UWB_ROLE_TAG) ? "TAG" : "ANCHOR",
             (compiled_role == UWB_ROLE_TAG) ? "TAG" : "ANCHOR");
      
      if (g_sys_cfg.role != compiled_role) {
          printf("[TEST-RS] SUCCESS: Single Binary Runtime Role Switch Verified!\r\n");
      } else {
          printf("[TEST-RS] FAILED: Role did not switch.\r\n");
      }
      printf("[TEST-RS] Reverting for next run...\r\n");
      uwb_cfg_get_default(&g_sys_cfg, compiled_role);
      uwb_cfg_save(&g_sys_cfg);
  }
#endif

  uwb_cfg_print(&g_sys_cfg);

  // 2) MCU-level reset and initialization
  dw3000_reset();
  uwb_lib_init();

  // 3) Apply config to MAC layer
  apply_uwb_cfg(&g_sys_cfg);
  
  // 4) Background logic init
  twr_dist_filter_init();
  uwb_print_radio_params_3lines();

  printf("[STEP3] apply_uwb_cfg done\r\n");

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1_BOOST);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV6;
  RCC_OscInitStruct.PLL.PLLN = 85;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
