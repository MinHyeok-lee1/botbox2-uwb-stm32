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
#include "can_controller.h"
#include "fmac.h"
#include "stm32g4xx_hal.h"
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
#include <unistd.h>

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

// ===== DS-TWR Additional Timing =====
#define RESP_RX_TO_FINAL_TX_DLY_UUS  1500   // TAG: delay from RESP RX to FINAL TX (reduced from 3000)
#define FINAL_RX_TIMEOUT_UUS         8000   // ANCHOR: timeout waiting for FINAL (bumped from 5000 for mixed-UWB window margin)

#define TX_ANT_DLY 16385
#define RX_ANT_DLY 16385

// ===== Operations Tuning =====
// ===== Operations Tuning =====
#define ANCHOR_LINK_TIMEOUT_MS  5000U  // Initial threshold for "Lost" state (ms)
#define ANCHOR_LOST_LOG_MS      5000U  // Interval for "Still Lost" reminders (ms)
#define ANCHOR_LP_TIMEOUT_MS   60000U  // Time before entering Low Power Listen (ms)
#define TAG_RANGING_RATE_HZ    6U    // Target Frequency (Full Scans per second)
#define TAG_ANCHOR_SLOT_MS    55U    // Per-anchor baseline budget (includes retries)
#define TAG_MIN_GAP_MS         2U    // Safety floor between scans (ms)
#define TAG_SLOT2_GUARD_US   10000U   // [TEST] Decisive 10ms guard delay before Slot 2 poll
#define TAG_SLOT3_GUARD_US   10000U   // [TEST] Decisive 10ms guard delay before Slot 3 poll
#define ANCHOR_LP_WAKE_INTERVAL_MS  1000U // Duty cycle period (ms)
#define ANCHOR_LP_LISTEN_WINDOW_MS   100U // Active RX window (ms)

#define TAG_SYS_STATUS_HYGIENE_BIT_MASK (SYS_STATUS_TXFRS_BIT_MASK | \
                                          SYS_STATUS_RXFCG_BIT_MASK | \
                                          SYS_STATUS_ALL_RX_TO      | \
                                          SYS_STATUS_ALL_RX_ERR     | \
                                          SYS_STATUS_AFFREJ_BIT_MASK)

/* ANCHOR Status Hygiene: Complete bitmask to ensure cold-start for every wait-for-POLL scan */
#define ANC_SYS_STATUS_HYGIENE_BIT_MASK (SYS_STATUS_TXFRS_BIT_MASK | \
                                          SYS_STATUS_RXFCG_BIT_MASK | \
                                          SYS_STATUS_ALL_RX_ERR     | \
                                          SYS_STATUS_ALL_RX_TO      | \
                                          SYS_STATUS_AFFREJ_BIT_MASK)

#ifndef UWB_ONE_WAY_BENCHMARK
#define UWB_ONE_WAY_BENCHMARK 0
#endif
 
#ifndef UWB_ONE_WAY_BENCHMARK_HZ
#define UWB_ONE_WAY_BENCHMARK_HZ 6U
#endif
 
#ifndef DS_RUNTIME_QUIET_MEASURE
#define DS_RUNTIME_QUIET_MEASURE 0
#endif
 
#ifndef DS_RUNTIME_TRACK_CLEAN_CEILING
#define DS_RUNTIME_TRACK_CLEAN_CEILING 1
#endif

#ifndef DS_RUNTIME_INSTRUMENTATION
#define DS_RUNTIME_INSTRUMENTATION 0
#endif

#ifndef DS_BOUNDED_SCHEDULER
#define DS_BOUNDED_SCHEDULER 0
#endif

#ifndef DS_ANCHOR_OBS_SUMMARY
#define DS_ANCHOR_OBS_SUMMARY 0
#endif
 
typedef enum {
    ANCHOR_STATE_ACTIVE = 0,
    ANCHOR_STATE_LINK_LOST,
    ANCHOR_STATE_LOW_POWER_LISTEN,
    ANCHOR_STATE_DEEP_SLEEP
} anchor_runtime_state_t;

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

#define DS_TWR_VERBOSE 0

#ifndef UWB_RUNTIME_VERBOSE_LOGS
#define UWB_RUNTIME_VERBOSE_LOGS 0
#endif

#ifndef UWB_DISTANCE_CAN_LOGS
#define UWB_DISTANCE_CAN_LOGS 1
#endif

union UwbPacket {
  struct __attribute__((__packed__)) {
    uint8_t start;
    int16_t distance;
    uint8_t checksum;
  };
  uint8_t byteStream[4];
};

/* USER CODE END Includes */
void Error_Handler(void);

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
#define DW3000_CS_PORT     UWB_SPI_CS_GPIO_Port
#define DW3000_CS_PIN      UWB_SPI_CS_Pin

#define DW3000_RST_PORT    UWB_RESET_GPIO_Port
#define DW3000_RST_PIN     UWB_RESET_Pin
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

// ===== DS-TWR FINAL frame layout =====
#define FINAL_DST_PAN_IDX            3
#define FINAL_DST_ADDR_IDX           5
#define FINAL_SRC_ADDR_IDX           7
#define FINAL_TYPE_IDX               9
#define FINAL_POLL_TX_TS_IDX        10   // 4 bytes
#define FINAL_RESP_RX_TS_IDX        14   // 4 bytes
#define FINAL_FINAL_TX_TS_IDX       18   // 4 bytes
#define FINAL_MSG_MIN_LEN           22   // Minimum valid FINAL frame length (header 10 + 3x4 TS)

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

/* TAG poll transmission observability */
typedef struct {
    uint8_t  sn;
    uint16_t target_aid;
    uint16_t pan_id;
    uint16_t dst_short;
    uint16_t src_short;
    uint8_t  header[12];
} tag_poll_snapshot_t;

static tag_poll_snapshot_t g_poll_ring[3];
static uint8_t g_poll_ring_idx = 0;

static void tag_poll_snapshot_record(uint16_t target_aid, uint8_t sn, const uint8_t *msg, uint32_t len)
{
    tag_poll_snapshot_t *s = &g_poll_ring[g_poll_ring_idx];
    s->sn         = sn;
    s->target_aid = target_aid;
    s->pan_id     = (uint16_t)msg[3] | ((uint16_t)msg[4] << 8);
    s->dst_short  = (uint16_t)msg[5] | ((uint16_t)msg[6] << 8);
    s->src_short  = (uint16_t)msg[7] | ((uint16_t)msg[8] << 8);
    
    uint32_t to_copy = (len > 12U) ? 12U : len;
    for (uint32_t i = 0; i < to_copy; i++) {
        s->header[i] = msg[i];
    }
    
    g_poll_ring_idx = (g_poll_ring_idx + 1) % 3;
}

static void tag_poll_snapshot_dump(void)
{
#if UWB_RUNTIME_VERBOSE_LOGS
    printf("[TAG][TXLOG] Last 3 Polls (Newest last):\r\n");
    for (int i = 0; i < 3; i++) {
        /* Print from oldest to newest */
        int idx = (g_poll_ring_idx + i) % 3;
        tag_poll_snapshot_t *s = &g_poll_ring[idx];
        if (s->sn == 0 && s->target_aid == 0) continue;
        
        printf("  [%d] SN=%u AID=0x%04X PAN=0x%04X DST=0x%04X SRC=0x%04X HEX:", 
               i, s->sn, s->target_aid, s->pan_id, s->dst_short, s->src_short);
        for (int j = 0; j < 12; j++) {
            printf(" %02X", s->header[j]);
        }
        printf("\r\n");
    }
#endif
}

/* Anchor sequence gap / miss observability */
typedef struct {
    char     cls[4]; /* "TO", "REJ", "ERR" */
    uint32_t st;
} anc_terminal_snapshot_t;

static anc_terminal_snapshot_t g_anc_ring[16];
static uint8_t g_anc_ring_idx = 0;
static uint8_t g_last_succ_sn = 0;
static bool    g_has_first_succ = false;

static void anc_terminal_snapshot_record(const char *cls, uint32_t status)
{
    anc_terminal_snapshot_t *s = &g_anc_ring[g_anc_ring_idx];
    for (int i = 0; i < 3; i++) s->cls[i] = cls[i];
    s->cls[3] = '\0';
    s->st = status;
    g_anc_ring_idx = (g_anc_ring_idx + 1) % 16;
}

static void anc_terminal_snapshot_clear(void)
{
    for (int i = 0; i < 16; i++) g_anc_ring[i].cls[0] = '\0';
    g_anc_ring_idx = 0;
}

static void anc_terminal_snapshot_dump(uint8_t prev_sn, uint8_t curr_sn)
{
#if UWB_RUNTIME_VERBOSE_LOGS
    printf("[TWR][ANC][MISS] AID=0x%04X prev=%u curr=%u gap=%u\r\n",
           (unsigned int)g_sys_cfg.short_addr, (unsigned int)prev_sn, (unsigned int)curr_sn, 
           (unsigned int)(uint8_t)(curr_sn - prev_sn));
    printf("  [MISS_CTX] Outcomes between successes (oldest first):\r\n");
    for (int i = 0; i < 16; i++) {
        anc_terminal_snapshot_t *s = &g_anc_ring[(g_anc_ring_idx + i) % 16];
        if (s->cls[0] == '\0') continue;
        printf("    %s st=0x%08lX\r\n", s->cls, (unsigned long)s->st);
    }
#endif
}

static void anc_silent_epoch_dump(uint32_t silent_ms)
{
#if UWB_RUNTIME_VERBOSE_LOGS
    printf("[TWR][ANC][SILENT] AID=0x%04X silent_ms=%lu last_sn=%u\r\n",
           (unsigned int)g_sys_cfg.short_addr, (unsigned long)silent_ms, (unsigned int)g_last_succ_sn);
    printf("  [SILENT_CTX] Outcomes during silence (oldest first):\r\n");
    for (int i = 0; i < 16; i++) {
        anc_terminal_snapshot_t *s = &g_anc_ring[(g_anc_ring_idx + i) % 16];
        if (s->cls[0] == '\0') continue;
        printf("    %s st=0x%08lX\r\n", s->cls, (unsigned long)s->st);
    }
#endif
}

static uint32_t g_last_succ_tick = 0;
static bool     g_silent_epoch_dumped = false;
#define ANC_SILENT_EPOCH_THRESHOLD_MS 2000U

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

#if UWB_ONE_WAY_BENCHMARK
#define ONEWAY_TYPE_IDX 9
#define ONEWAY_SEQ_IDX  10
#define ONEWAY_MSG_LEN  12U

static uint8_t tx_oneway_msg[] = {
  0x41, 0x88, 0x00,              // FC, SN
  0x00, 0x00,                    // DEST PAN (patched)
  0x00, 0x00,                    // DEST SHORT ADDR (TAG, patched)
  0x00, 0x00,                    // SRC SHORT ADDR (ANCHOR, patched)
  0xC3,                          // one-way benchmark type
  0x00, 0x00                     // benchmark sequence u16 (patched)
};
#endif

// DS-TWR FINAL message template
static uint8_t tx_final_msg[] = {
  0x41, 0x88, 0x00,              // 0..2   FC, SN
  0x00, 0x00,                    // 3..4   DST PAN (patched)
  0x00, 0x00,                    // 5..6   DST short addr (anchor, patched)
  0x00, 0x00,                    // 7..8   SRC short addr (tag, patched)
  0xE2,                          // 9      FINAL type
  0,0,0,0,                       // 10..13 poll_tx_ts (lo32)
  0,0,0,0,                       // 14..17 resp_rx_ts (lo32)
  0,0,0,0,                       // 18..21 final_tx_ts (lo32, predicted)
  0x00                           // 22     reserved
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

    // Measurement mode extensions
    uint32_t total_cycles;
    uint32_t no_rx_status_cnt;
    uint32_t current_fail_burst;
    uint32_t longest_fail_burst;
    uint32_t fail_cnt;   // Software-level ranging failures (timeouts/bad seq)
} twr_stats_t;

static twr_stats_t g_tag_stat;
static twr_stats_t g_anc_stat;
static twr_stats_t g_tag_snap;
static twr_stats_t g_anc_snap;

#if DS_ANCHOR_OBS_SUMMARY
typedef struct {
    uint32_t matched_poll_count;
    uint32_t response_tx_attempt_count;
    uint32_t response_tx_ok_count;
    uint32_t final_rx_ok_count;
    uint32_t final_rx_timeout_count;
    uint32_t final_rx_mismatch_count;
    uint32_t final_rx_bad_len_count;
    uint32_t final_rx_err_count;
    uint32_t final_rx_other_count;
    uint32_t rx_err_count;
    uint32_t rx_to_count;
    uint32_t affrej_count;
    uint32_t rejected_nonmatching_poll_count;
    uint32_t last_print_ms;
} anc_obs_summary_t;

static anc_obs_summary_t g_anc_obs_summary;

static void anc_obs_summary_maybe_print(uint32_t interval_ms)
{
    uint32_t now = HAL_GetTick();
    if ((uint32_t)(now - g_anc_obs_summary.last_print_ms) >= interval_ms) {
        g_anc_obs_summary.last_print_ms = now;
        printf("[ANC][OBS] poll:%lu resp_try:%lu resp_ok:%lu final_ok:%lu final_to:%lu final_mis:%lu final_len:%lu final_err:%lu final_other:%lu rxerr:%lu rxto:%lu affrej:%lu rej:%lu\r\n",
               (unsigned long)g_anc_obs_summary.matched_poll_count,
               (unsigned long)g_anc_obs_summary.response_tx_attempt_count,
               (unsigned long)g_anc_obs_summary.response_tx_ok_count,
               (unsigned long)g_anc_obs_summary.final_rx_ok_count,
               (unsigned long)g_anc_obs_summary.final_rx_timeout_count,
               (unsigned long)g_anc_obs_summary.final_rx_mismatch_count,
               (unsigned long)g_anc_obs_summary.final_rx_bad_len_count,
               (unsigned long)g_anc_obs_summary.final_rx_err_count,
               (unsigned long)g_anc_obs_summary.final_rx_other_count,
               (unsigned long)g_anc_obs_summary.rx_err_count,
               (unsigned long)g_anc_obs_summary.rx_to_count,
               (unsigned long)g_anc_obs_summary.affrej_count,
               (unsigned long)g_anc_obs_summary.rejected_nonmatching_poll_count);
    }
}
#endif

/* AFFREJ burst guard */
static uint32_t g_affrej_window_ms = 0U;
static uint32_t g_affrej_window_cnt = 0U;

// Δ1: STAT 로그 출력 - 구현
static void twr_stats_maybe_print(const char* prefix, twr_stats_t* s, uint32_t interval_ms)
{
#if UWB_RUNTIME_VERBOSE_LOGS
    uint32_t now = HAL_GetTick();
    if ((uint32_t)(now - s->last_print_ms) >= interval_ms) {
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
#endif
}

// ===== SS-TWR =====
static inline void put_ts4(uint8_t *dst4, uint64_t ts) {
  dst4[0] = (uint8_t) ts;
  dst4[1] = (uint8_t)(ts >> 8);
  dst4[2] = (uint8_t)(ts >> 16);
  dst4[3] = (uint8_t)(ts >> 24);
}

// ===== State =====
static uint8_t  tag_sn = 0;
static uint8_t  rx_buf[64];

static inline void set_u16_le(uint8_t *p, uint16_t v) {
    p[0] = (uint8_t)(v & 0xFF);
    p[1] = (uint8_t)((v >> 8) & 0xFF);
}

static const char* tag_fail_cat_from_status(uint32_t st) {
    if ((st & SYS_STATUS_RXFCG_BIT_MASK) != 0U) return "rx_seen_unhandled";
    if ((st & SYS_STATUS_ARFE_BIT_MASK) != 0U)  return "rx_err_arfe";
    if ((st & SYS_STATUS_ALL_RX_TO) != 0U) {
        return ((st & SYS_STATUS_ALL_RX_ERR) != 0U) ? "rx_timeout_err" : "rx_timeout";
    }
    if ((st & SYS_STATUS_ALL_RX_ERR) != 0U) return "rx_error";
    return "no_progress";
}

#if UWB_RUNTIME_VERBOSE_LOGS
static const char* tag_rx_sub_from_capture(uint32_t first_rx_st, uint32_t last_rx_st, uint32_t flen) {
    const uint32_t cap_st = first_rx_st | last_rx_st;
    if (((cap_st & SYS_STATUS_RXFCG_BIT_MASK) != 0U) || (flen != 0U)) return "frame_seen";
    if ((cap_st & SYS_STATUS_ALL_RX_ERR) != 0U) return "rx_err";
    if ((cap_st & SYS_STATUS_ALL_RX_TO) != 0U) return "to_only";
    return "no_rx_status";
}
#endif

static void tag_log_fail_cat(uint16_t aid, uint8_t sn, const char *cat, uint32_t armed, uint32_t st, uint32_t flen,
                             uint32_t age_us, uint32_t first_rx_st, uint32_t last_rx_st,
                             uint32_t rx_state0, uint32_t rx_state1)
{
    if (strcmp(cat, "no_rx_status") == 0) {
        g_tag_stat.no_rx_status_cnt++;
    }
    if (st & SYS_STATUS_ALL_RX_ERR)      g_tag_stat.rx_err++;
    if (st & SYS_STATUS_AFFREJ_BIT_MASK) g_tag_stat.rx_affrej++;
#if !DS_RUNTIME_QUIET_MEASURE && (TAG_RANGING_RATE_HZ <= 64U)
#if UWB_RUNTIME_VERBOSE_LOGS
    const uint32_t seen = (((st & SYS_STATUS_RXFCG_BIT_MASK) != 0U) || (flen != 0U)) ? 1U : 0U;
    const uint32_t rxprog = first_rx_st | last_rx_st;
    const uint32_t state_changed = (rx_state0 != rx_state1) ? 1U : 0U;
#endif
#if UWB_RUNTIME_VERBOSE_LOGS
    printf("[TAG][FAILCAT] AID=0x%04X SN=%u cat=%s rxsub=%s armed=%lu seen=%lu st=0x%08lX flen=%lu age=%lu rxprog=0x%08lX first=0x%08lX last=0x%08lX rstate0=0x%08lX rstate1=0x%08lX schg=%lu\r\n",
           (unsigned int)aid,
           (unsigned int)sn,
           cat,
           tag_rx_sub_from_capture(first_rx_st, last_rx_st, flen),
           (unsigned long)armed,
           (unsigned long)seen,
           (unsigned long)st,
           (unsigned long)flen,
           (unsigned long)age_us,
           (unsigned long)rxprog,
           (unsigned long)first_rx_st,
           (unsigned long)last_rx_st,
           (unsigned long)rx_state0,
           (unsigned long)rx_state1,
           (unsigned long)state_changed);
#endif
    
    /* Dump recent poll history to correlate with the failure */
    tag_poll_snapshot_dump();
#else
    (void)aid; (void)sn; (void)cat; (void)armed; (void)st; (void)flen; (void)age_us;
    (void)first_rx_st; (void)last_rx_st; (void)rx_state0; (void)rx_state1;
#endif
}

#if UWB_ONE_WAY_BENCHMARK
typedef struct {
    uint32_t attempted;
    uint32_t tx_done;
    uint32_t tx_fail;
    uint32_t accepted;
    uint32_t missed;
    uint32_t invalid;
    uint32_t rx_err;
    uint32_t longest_miss_burst;
    uint32_t consecutive_miss;
    uint32_t last_print_ms;
    uint16_t last_seq;
    bool synced;

    /* Pinpoint Logging Caps */
    uint32_t sync_log_cnt;
    uint32_t susp_log_cnt;
    uint32_t inv_log_cnt;
} oneway_bench_stats_t;

static oneway_bench_stats_t g_oneway_bench;

static uint32_t oneway_benchmark_period_ms(void)
{
    uint32_t hz = UWB_ONE_WAY_BENCHMARK_HZ;
    if (hz == 0U) hz = 1U;
    uint32_t period_ms = 1000U / hz;
    if (period_ms == 0U) period_ms = 1U;
    return period_ms;
}

static void oneway_anchor_stats_maybe_print(void)
{
#if UWB_RUNTIME_VERBOSE_LOGS
    uint32_t now = HAL_GetTick();
    if ((uint32_t)(now - g_oneway_bench.last_print_ms) >= 1000U) {
        g_oneway_bench.last_print_ms = now;
        printf("[ANC][1WAY] hz=%u attempted=%lu txdone=%lu txfail=%lu sn=%u\r\n",
               (unsigned)UWB_ONE_WAY_BENCHMARK_HZ,
               (unsigned long)g_oneway_bench.attempted,
               (unsigned long)g_oneway_bench.tx_done,
               (unsigned long)g_oneway_bench.tx_fail,
               (unsigned)g_oneway_bench.last_seq);
    }
#endif
}

static void oneway_tag_stats_maybe_print(void)
{
#if UWB_RUNTIME_VERBOSE_LOGS
    uint32_t now = HAL_GetTick();
    if ((uint32_t)(now - g_oneway_bench.last_print_ms) >= 1000U) {
        g_oneway_bench.last_print_ms = now;
        printf("[TAG][1WAY] hz=%u acc=%lu miss=%lu inv=%lu rxerr=%lu longest=%lu last=%u sync=%u\r\n",
               (unsigned)UWB_ONE_WAY_BENCHMARK_HZ,
               (unsigned long)g_oneway_bench.accepted,
               (unsigned long)g_oneway_bench.missed,
               (unsigned long)g_oneway_bench.invalid,
               (unsigned long)g_oneway_bench.rx_err,
               (unsigned long)g_oneway_bench.longest_miss_burst,
               (unsigned)g_oneway_bench.last_seq,
               g_oneway_bench.synced ? 1U : 0U);
    }
#endif
}

void oneway_benchmark_anchor_main(void)
{
    const uint32_t period_ms = oneway_benchmark_period_ms();
    uint16_t seq = 0U;

    printf("[ANC][1WAY][APP] start hz=%u dst=0x%04X src=0x%04X\r\n",
           (unsigned)UWB_ONE_WAY_BENCHMARK_HZ,
           (unsigned)TAG_SHORT_ADDR,
           (unsigned)_mac.addr);

    while (1) {
        uint32_t t_start = HAL_GetTick();
        uint8_t sn = (uint8_t)(seq & 0xFFU);

        dwt_forcetrxoff();
        dwt_write32bitreg(SYS_STATUS_ID,
                          SYS_STATUS_TXFRS_BIT_MASK |
                          SYS_STATUS_RXFCG_BIT_MASK |
                          SYS_STATUS_ALL_RX_TO |
                          SYS_STATUS_ALL_RX_ERR);

        tx_oneway_msg[ALL_MSG_SN_IDX] = sn;
        set_u16_le(&tx_oneway_msg[POLL_DST_PAN_IDX],  _mac.panId);
        set_u16_le(&tx_oneway_msg[POLL_DST_ADDR_IDX], TAG_SHORT_ADDR);
        set_u16_le(&tx_oneway_msg[POLL_SRC_ADDR_IDX], _mac.addr);
        set_u16_le(&tx_oneway_msg[ONEWAY_SEQ_IDX], seq);

        g_oneway_bench.attempted++;
        g_oneway_bench.last_seq = seq;

        dwt_writetxdata(sizeof(tx_oneway_msg), tx_oneway_msg, 0);
        dwt_writetxfctrl(sizeof(tx_oneway_msg) + 2U, 0, 1);

        if (dwt_starttx(DWT_START_TX_IMMEDIATE) == DWT_SUCCESS) {
            uint32_t tx_wait_start = micros();
            uint32_t st = 0U;
            do {
                st = dwt_read32bitreg(SYS_STATUS_ID);
            } while (((st & SYS_STATUS_TXFRS_BIT_MASK) == 0U) &&
                     ((uint32_t)(micros() - tx_wait_start) <= 2000U));

            if ((st & SYS_STATUS_TXFRS_BIT_MASK) != 0U) {
                g_oneway_bench.tx_done++;
                dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_TXFRS_BIT_MASK);
            } else {
                g_oneway_bench.tx_fail++;
                dwt_forcetrxoff();
            }
        } else {
            g_oneway_bench.tx_fail++;
            dwt_forcetrxoff();
        }

        seq++;
        oneway_anchor_stats_maybe_print();

        uint32_t elapsed_ms = (uint32_t)(HAL_GetTick() - t_start);
        HAL_Delay((elapsed_ms < period_ms) ? (period_ms - elapsed_ms) : 1U);
    }
}

void oneway_benchmark_tag_main(void)
{
    printf("[TAG][1WAY][APP] start hz=%u dst=0x%04X expected_src=0x%04X\r\n",
           (unsigned)UWB_ONE_WAY_BENCHMARK_HZ,
           (unsigned)_mac.addr,
           (unsigned)g_sys_cfg.anchor_ids[0]);

    memset(&g_oneway_bench, 0, sizeof(g_oneway_bench));

    dwt_forcetrxoff();
    dwt_write32bitreg(SYS_STATUS_ID,
                      SYS_STATUS_ALL_RX_GOOD |
                      SYS_STATUS_ALL_RX_ERR |
                      SYS_STATUS_ALL_RX_TO |
                      SYS_STATUS_TXFRS_BIT_MASK |
                      SYS_STATUS_AFFREJ_BIT_MASK);
    dwt_setrxtimeout(0U);
    dwt_rxenable(DWT_START_RX_IMMEDIATE);

    while (1) {
        uint32_t st = dwt_read32bitreg(SYS_STATUS_ID);

        if ((st & SYS_STATUS_RXFCG_BIT_MASK) != 0U) {
            uint32_t flen = dwt_read32bitreg(RX_FINFO_ID) & RXFLEN_MASK;
            if (flen <= sizeof(rx_buf)) {
                dwt_readrxdata(rx_buf, flen, 0);
            }
            dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_ALL_RX_GOOD);

            bool len_ok = (flen >= ONEWAY_MSG_LEN) && (flen <= sizeof(rx_buf));
            bool fc_ok = len_ok && (rx_buf[0] == 0x41U) && (rx_buf[1] == 0x88U);
            bool type_ok = len_ok && (rx_buf[ONEWAY_TYPE_IDX] == 0xC3U);
            uint16_t pan_rx = len_ok ? ((uint16_t)rx_buf[POLL_DST_PAN_IDX] |
                                        ((uint16_t)rx_buf[POLL_DST_PAN_IDX + 1] << 8)) : 0xFFFFU;
            uint16_t dst_rx = len_ok ? ((uint16_t)rx_buf[POLL_DST_ADDR_IDX] |
                                        ((uint16_t)rx_buf[POLL_DST_ADDR_IDX + 1] << 8)) : 0xFFFFU;
            uint16_t src_rx = len_ok ? ((uint16_t)rx_buf[POLL_SRC_ADDR_IDX] |
                                        ((uint16_t)rx_buf[POLL_SRC_ADDR_IDX + 1] << 8)) : 0xFFFFU;
            uint16_t seq_rx = len_ok ? ((uint16_t)rx_buf[ONEWAY_SEQ_IDX] |
                                        ((uint16_t)rx_buf[ONEWAY_SEQ_IDX + 1] << 8)) : 0U;
            bool addr_ok = (pan_rx == _mac.panId) && (dst_rx == _mac.addr) &&
                           (src_rx == g_sys_cfg.anchor_ids[0]);

            const char* inv_reason = NULL;

            if (fc_ok && type_ok && addr_ok) {
                bool accept_frame = false;
                if (g_oneway_bench.synced) {
                    uint16_t gap = (uint16_t)(seq_rx - g_oneway_bench.last_seq);
                    if (gap == 0U) {
                        inv_reason = "dup";
                        g_oneway_bench.invalid++;
                    } else if (gap < 0x8000U) {
                        uint32_t skipped = (uint32_t)(gap - 1U);
                        if (skipped > 0U) {
                            g_oneway_bench.missed += skipped;
                            g_oneway_bench.consecutive_miss = skipped;
                            if (skipped > g_oneway_bench.longest_miss_burst) {
                                g_oneway_bench.longest_miss_burst = skipped;
                            }
                            if (g_oneway_bench.susp_log_cnt < 10U) {
                                g_oneway_bench.susp_log_cnt++;
                                printf("[TAG][1WAY][SUSP] forward_gap=%lu seq_rx=%u last_seq=%u\r\n",
                                       (unsigned long)skipped, (unsigned)seq_rx, (unsigned)g_oneway_bench.last_seq);
                            }
                        } else {
                            g_oneway_bench.consecutive_miss = 0U;
                        }
                        accept_frame = true;
                    } else {
                        inv_reason = "backward";
                        g_oneway_bench.invalid++;
                    }
                } else {
                    g_oneway_bench.synced = true;
                    g_oneway_bench.consecutive_miss = 0U;
                    accept_frame = true;
                    if (g_oneway_bench.sync_log_cnt < 5U) {
                        g_oneway_bench.sync_log_cnt++;
                        printf("[TAG][1WAY][SYNC] first_seq=%u PAN=0x%04X SRC=0x%04X flen=%lu\r\n",
                               (unsigned)seq_rx, (unsigned)pan_rx, (unsigned)src_rx, (unsigned long)flen);
                    }
                }

                if (accept_frame) {
                    g_oneway_bench.accepted++;
                    g_oneway_bench.last_seq = seq_rx;
                }
            } else {
                g_oneway_bench.invalid++;
                if (!len_ok) inv_reason = "bad_len";
                else if (!fc_ok) inv_reason = "wrong_fc";
                else if (!type_ok) inv_reason = "wrong_type";
                else inv_reason = "wrong_addr"; // PAN, DST, or SRC
            }

            if (inv_reason != NULL && g_oneway_bench.inv_log_cnt < 10U) {
                g_oneway_bench.inv_log_cnt++;
                printf("[TAG][1WAY][INV] r=%s seq_rx=%u flen=%lu st=0x%08lX last=%u\r\n",
                       inv_reason, (unsigned)seq_rx, (unsigned long)flen, (unsigned long)st, (unsigned)g_oneway_bench.last_seq);
            }

            dwt_forcetrxoff();
            dwt_write32bitreg(SYS_STATUS_ID, ANC_SYS_STATUS_HYGIENE_BIT_MASK);
            dwt_rxenable(DWT_START_RX_IMMEDIATE);
        } else if ((st & (SYS_STATUS_ALL_RX_ERR | SYS_STATUS_ALL_RX_TO | SYS_STATUS_AFFREJ_BIT_MASK)) != 0U) {
            g_oneway_bench.rx_err++;
            dwt_forcetrxoff();
            dwt_write32bitreg(SYS_STATUS_ID,
                              SYS_STATUS_ALL_RX_ERR |
                              SYS_STATUS_ALL_RX_TO |
                              SYS_STATUS_AFFREJ_BIT_MASK);
            dwt_rxenable(DWT_START_RX_IMMEDIATE);
        }

        oneway_tag_stats_maybe_print();
    }
}
#endif

static bool poll_one_aid(uint16_t dst_short, float *out_m) {
#if DS_RUNTIME_INSTRUMENTATION
    uint32_t t_hygiene = 0;
#endif
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
#if DS_RUNTIME_INSTRUMENTATION
    uint32_t t_poll_tx_end = 0, t_resp_detect = 0, t_resp_rx_end = 0, t_final_tx_start = 0, t_final_tx_end = 0;
#endif

    tx_poll_msg[ALL_MSG_SN_IDX] = this_sn;
    set_u16_le(&tx_poll_msg[POLL_DST_PAN_IDX],  _mac.panId);
    set_u16_le(&tx_poll_msg[POLL_DST_ADDR_IDX], dst_short);
    set_u16_le(&tx_poll_msg[POLL_SRC_ADDR_IDX], _mac.addr);
    
    /* Record snapshot for failure correlation */
    tag_poll_snapshot_record(dst_short, this_sn, tx_poll_msg, sizeof(tx_poll_msg));

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
#if !DS_RUNTIME_QUIET_MEASURE
#if defined(DS_TWR_VERBOSE) && DS_TWR_VERBOSE
        printf("[TWR][TAG][TXFAIL] AID=0x%04X SN=%u ret=%d st=0x%08lX\r\n",
             (unsigned int)dst_short,
             (unsigned int)this_sn,
             (int)ret,
             (unsigned long)st_now);
#endif
#endif
        tag_log_fail_cat(dst_short, this_sn, "tx_start", 0U, st_now, 0U, 0U, 0U, 0U, 0U, 0U);
        return false;
    }

    // Save POLL TX timestamp right after TXFRS (MUST observe TXFRS)
    uint32_t poll_tx_ts_saved = 0;
    bool txfrs_ok = false;
#if DS_RUNTIME_INSTRUMENTATION
    uint32_t t_hygiene_end = 0;
#endif
    uint32_t tx_wait_start = micros();
    {
      while (1) {
        uint32_t st_tx = dwt_read32bitreg(SYS_STATUS_ID);

        if (st_tx & SYS_STATUS_TXFRS_BIT_MASK) {
          poll_tx_ts_saved = dwt_readtxtimestamplo32();
          /* Post-TX Hygiene: Clear TXFRS, all POLL-side TX-progress residue (0x70: TXPHS/TXPRS/TXFRB),
           * and any transient RX bits (AFFREJ/ERR) that tripped during the transition. */
          dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_TXFRS_BIT_MASK | 
                                          SYS_STATUS_TXPHS_BIT_MASK | 
                                          SYS_STATUS_TXPRS_BIT_MASK | 
                                          SYS_STATUS_TXFRB_BIT_MASK | 
                                          SYS_STATUS_AFFREJ_BIT_MASK | 
                                          SYS_STATUS_ALL_RX_ERR);
          txfrs_ok = true;
          break;
        }

        // safety timeout to avoid deadlock (e.g. 2ms)
        if ((micros() - tx_wait_start) > 2000U) {
          return false;
        }
      }
#if DS_RUNTIME_INSTRUMENTATION
      t_poll_tx_end = micros();
      t_hygiene_end = t_poll_tx_end;
#endif
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
#if !DS_RUNTIME_QUIET_MEASURE
#if defined(DS_TWR_VERBOSE) && DS_TWR_VERBOSE
        printf("[TWR][TAG][TXFRS_TO] AID=0x%04X SN=%u st=0x%08lX\r\n",
             (unsigned int)dst_short,
             (unsigned int)this_sn,
             (unsigned long)st_now);
#endif
#endif
        tag_log_fail_cat(dst_short, this_sn, "txfrs_timeout", 0U, st_now, 0U, 0U, 0U, 0U, 0U, 0U);
        return false;
    }

    const uint32_t rx_state0 = dwt_read32bitreg(SYS_STATE_LO_ID);
    const uint32_t t_start   = micros();
    const uint32_t window_us = RESP_RX_TIMEOUT_UUS; // NOTE: unit-mix remains for now

    uint32_t last_st = 0U;
    uint32_t last_flen = 0U;
    uint32_t first_rx_st = 0U;
    uint32_t last_rx_st = 0U;
    
    
#if DS_RUNTIME_INSTRUMENTATION
    uint32_t t_loop_start = micros();
    uint32_t st_loop_first = 0, t_loop_first = 0;
    uint32_t st_first_nz = 0, t_first_nz = 0;
#endif

    while (true) {
        if ((uint32_t)(micros() - t_start) > 5000U) {
            uint32_t st_now = dwt_read32bitreg(SYS_STATUS_ID) | last_st;
            uint32_t rx_state1 = dwt_read32bitreg(SYS_STATE_LO_ID);
            dwt_forcetrxoff();
            dwt_write32bitreg(SYS_STATUS_ID,
                SYS_STATUS_ALL_RX_ERR | SYS_STATUS_ALL_RX_TO |
                SYS_STATUS_RXFCG_BIT_MASK | SYS_STATUS_TXFRS_BIT_MASK);
            tag_log_fail_cat(dst_short, this_sn, tag_fail_cat_from_status(st_now),
                             1U, st_now, last_flen, (uint32_t)(micros() - t_start),
                             first_rx_st, last_rx_st, rx_state0, rx_state1);

#if DS_RUNTIME_INSTRUMENTATION
            if (first_rx_st == 0U && t_hygiene != 0U) {
                /* INSTR: Separated multi-stage snapshots for seen=0 anomalies */
                printf("[TWR][TAG][INSTR] SN=%u h2l=%luus l2f=%luus stL=0x%08lX stNZ=0x%08lX(%s%s) stF=0x%08lX(%s) stateF=0x%08lX\r\n",
                       (unsigned int)this_sn,
                       (unsigned long)(t_loop_start - t_hygiene),
                       (unsigned long)(t_loop_first - t_loop_start),
                       (unsigned long)st_loop_first,
                       (unsigned long)st_first_nz,
                       (st_first_nz & SYS_STATUS_ARFE_BIT_MASK) ? "ARFE " : "",
                       (st_first_nz & SYS_STATUS_ALL_RX_ERR) ? "RXERR " : "",
                       (unsigned long)st_now,
                       tag_fail_cat_from_status(st_now),
                       (unsigned long)rx_state1);
            }
#endif
            return false;
        }

        uint32_t st;
        do {
            uint32_t raw_st = dwt_read32bitreg(SYS_STATUS_ID);
#if DS_RUNTIME_INSTRUMENTATION
            if (t_loop_first == 0U) {
                t_loop_first = micros();
                st_loop_first = raw_st;
            }
            if (st_first_nz == 0U && raw_st != 0U) {
                st_first_nz = raw_st;
                t_first_nz = micros();
            }
            uint32_t raw_rx_st = raw_st & rx_watch_mask;
            if (raw_rx_st != 0U) {
                if (first_rx_st == 0U) {
                    first_rx_st = raw_rx_st;
                    t_resp_detect = micros();
                }
                last_rx_st = raw_rx_st;
            }
#endif
            st = raw_st;
            if ((micros() - t_start) >= window_us) {
                st |= SYS_STATUS_ALL_RX_TO;
                break;
            }
        } while (!(st & (SYS_STATUS_RXFCG_BIT_MASK | SYS_STATUS_ALL_RX_TO | SYS_STATUS_ALL_RX_ERR)));
        last_st = st;

        if (st & SYS_STATUS_RXFCG_BIT_MASK) {
            dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_RXFCG_BIT_MASK);

            uint32_t flen = dwt_read32bitreg(RX_FINFO_ID) & RXFLEN_MASK;
            last_flen = flen;
            if (flen > sizeof(rx_buf)) {
                uint32_t elapsed = micros() - t_start;
                if (elapsed >= window_us) {
                    uint32_t rx_state1 = dwt_read32bitreg(SYS_STATE_LO_ID);
                    dwt_forcetrxoff();
                    dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_RXFCG_BIT_MASK | SYS_STATUS_ALL_RX_ERR | SYS_STATUS_ALL_RX_TO);
                    tag_log_fail_cat(dst_short, this_sn, "bad_len", 1U, st, flen, elapsed,
                                     first_rx_st, last_rx_st, rx_state0, rx_state1);
                    return false;
                }
                dwt_setrxtimeout(window_us - elapsed);
                dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_ALL_RX_ERR | SYS_STATUS_ALL_RX_TO);
                /* Hardening: Always force TRX off before internal re-arm to clean hardware sequencer */
                dwt_forcetrxoff();
                dwt_rxenable(DWT_START_RX_IMMEDIATE);
                continue;
            }

            dwt_readrxdata(rx_buf, flen, 0);
#if DS_RUNTIME_INSTRUMENTATION
            t_resp_rx_end = micros();
#endif

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
                if (elapsed >= window_us) {
                    uint32_t rx_state1 = dwt_read32bitreg(SYS_STATE_LO_ID);
                    dwt_forcetrxoff();
                    dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_RXFCG_BIT_MASK | SYS_STATUS_ALL_RX_ERR | SYS_STATUS_ALL_RX_TO);
                    tag_log_fail_cat(dst_short, this_sn, "bad_resp", 1U, st, flen, elapsed,
                                     first_rx_st, last_rx_st, rx_state0, rx_state1);
                    return false;
                }
                dwt_setrxtimeout(window_us - elapsed);
                dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_ALL_RX_ERR | SYS_STATUS_ALL_RX_TO);
                /* Hardening: Always force TRX off before internal re-arm to clean hardware sequencer */
                dwt_forcetrxoff();
                dwt_rxenable(DWT_START_RX_IMMEDIATE);
                continue;
            }

            /* ============================================================= */
            /* DS-TWR branch: TAG sends FINAL, does NOT compute distance     */
            /* ============================================================= */
            if (g_sys_cfg.ranging_mode == UWB_RANGING_MODE_DS) {
                uint32_t poll_tx_ts = (poll_tx_ts_saved != 0U) ? poll_tx_ts_saved : dwt_readtxtimestamplo32();
                uint64_t resp_rx_ts_40 = get_rx_timestamp_u64();
                uint32_t resp_rx_ts = (uint32_t)(resp_rx_ts_40 & 0xFFFFFFFFUL);

                /* Compose FINAL message */
                tx_final_msg[ALL_MSG_SN_IDX] = this_sn;
                set_u16_le(&tx_final_msg[FINAL_DST_PAN_IDX],  _mac.panId);
                set_u16_le(&tx_final_msg[FINAL_DST_ADDR_IDX], dst_short);
                set_u16_le(&tx_final_msg[FINAL_SRC_ADDR_IDX], _mac.addr);

                /* Embed poll_tx_ts and resp_rx_ts */
                put_ts4(&tx_final_msg[FINAL_POLL_TX_TS_IDX], (uint64_t)poll_tx_ts);
                put_ts4(&tx_final_msg[FINAL_RESP_RX_TS_IDX], (uint64_t)resp_rx_ts);

                /* Delayed FINAL TX scheduling — use full 40-bit RX timestamp
                 * (matches anchor's proven RESP TX pattern) */
                uint32_t resp_rx_ts_h = (uint32_t)((resp_rx_ts_40 & 0xFFFFFFFFFFULL) >> 8);
                uint32_t final_tx_dly_h = (uint32_t)(((uint64_t)RESP_RX_TO_FINAL_TX_DLY_UUS * UUS_TO_DWT_TIME) >> 8);
                uint32_t final_target_h = resp_rx_ts_h + final_tx_dly_h;
                dwt_setdelayedtrxtime(final_target_h);

                /* Predict final_tx_ts and embed in FINAL payload */
                uint64_t final_tx_ts_64 = (((uint64_t)(final_target_h & 0xFFFFFFFEUL)) << 8) + (uint64_t)(TX_ANT_DLY);
                put_ts4(&tx_final_msg[FINAL_FINAL_TX_TS_IDX], final_tx_ts_64);

#if DS_RUNTIME_INSTRUMENTATION
                t_final_tx_start = micros();
#endif
                /* No printf before starttx — UART overhead breaks delayed TX budget */
                dwt_writetxdata(sizeof(tx_final_msg), tx_final_msg, 0);
                dwt_writetxfctrl(sizeof(tx_final_msg) + 2, 0, 1);

                if (dwt_starttx(DWT_START_TX_DELAYED) != DWT_SUCCESS) {
                    dwt_forcetrxoff();
                    return false;
                }

                /* Wait for FINAL TX complete */
                uint32_t st_final;
                do { st_final = dwt_read32bitreg(SYS_STATUS_ID); } while ((st_final & SYS_STATUS_TXFRS_BIT_MASK) == 0U);
#if DS_RUNTIME_INSTRUMENTATION
                t_final_tx_end = micros();
#endif
                dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_TXFRS_BIT_MASK);

                /* DS-TWR Success Cleanup: TAG does NOT compute distance. 
                 * Hardening: Explicit TRX off and status clear ensures no residue for next cycle. */
                dwt_forcetrxoff();
                dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_RXFCG_BIT_MASK | SYS_STATUS_ALL_RX_ERR | SYS_STATUS_ALL_RX_TO | SYS_STATUS_TXFRS_BIT_MASK);
                
#if UWB_RUNTIME_VERBOSE_LOGS
#if DS_RUNTIME_INSTRUMENTATION
                printf("[TAG][TRAN_DET] AID=0x%04X p2rd:%lu rxdur:%lu r2ft:%lu tot:%lu hyg:%lu\r\n",
                    (unsigned int)dst_short,
                    (unsigned long)(t_resp_detect - t_poll_tx_end),
                    (unsigned long)(t_resp_rx_end - t_resp_detect),
                    (unsigned long)(t_final_tx_start - t_resp_rx_end),
                    (unsigned long)(t_final_tx_end - t_poll_tx_end),
                    (unsigned long)(t_hygiene_end - tx_wait_start));
#endif
#endif

                *out_m = 0.0f;
                return true;
            }

            /* ============================================================= */
            /* SS-TWR branch: existing TAG distance calculation (unchanged)  */
            /* ============================================================= */
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
#if defined(TAG_VERBOSE_LOGS) && TAG_VERBOSE_LOGS
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
#endif


            // Δ2: DIAG sanity check
            bool diag_ok = twr_diag_is_sane(&diag);

            // Δ2: 품질 점수 계산
            twr_quality_t q = twr_quality_from_diag(&diag);


#if defined(TAG_VERBOSE_LOGS) && TAG_VERBOSE_LOGS
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
#endif
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

#if defined(TAG_VERBOSE_LOGS) && TAG_VERBOSE_LOGS
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
#endif
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

#if defined(TAG_VERBOSE_LOGS) && TAG_VERBOSE_LOGS
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
#endif

            // Distance filter update + ground-truth stats (if enabled).
            twr_dist_filter_update_and_log_mm_ex((uint16_t)dst_short, this_sn, (float)(dist * 1000.0),
                                                quality_ok, q_weight, quality_fail_reason);
        return true;
        } else {
            /* Debug-only: noisy during healthy operation (AFFREJ, timeout retries).
             * Re-enable with -DTAG_RX_FAIL_VERBOSE or #define TAG_RX_FAIL_VERBOSE */
#ifdef TAG_RX_FAIL_VERBOSE
            printf("[RX FAIL] st=0x%08lX\r\n", (unsigned long)st);
#endif

            // Hardening: Force TRX off and clear status for a clean loop-back or exit
            dwt_forcetrxoff();
            dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_ALL_RX_ERR | SYS_STATUS_ALL_RX_TO | SYS_STATUS_RXFCG_BIT_MASK);

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

/**
 * @brief Sends UWB distance data via CAN using the union UwbPacket format.
 *        Format: {start:0xAA, distance:int16_t, checksum:XOR}
 *        CAN ID: 0x200 + (aid & 0x0F)
 */
static void uwb_can_send_anchor_distance_mm(uint16_t aid, long dist_mm)
{
#if UWB_DISTANCE_CAN_LOGS
    union UwbPacket pkt;
    pkt.start = 0xAA;
    
    /* Clamp distance to int16 range */
    if (dist_mm > 32767L) dist_mm = 32767L;
    if (dist_mm < -32768L) dist_mm = -32768L;
    pkt.distance = (int16_t)dist_mm;
    
    pkt.checksum = pkt.byteStream[0] ^ pkt.byteStream[1] ^ pkt.byteStream[2];
    
    uint32_t can_id = 0x200U + (aid & 0x0FU);
    HAL_StatusTypeDef st = can_tx_msg(can_id, pkt.byteStream, 4);
    
#if UWB_RUNTIME_VERBOSE_LOGS
    if (st != HAL_OK) {
        printf("\r\n[CAN][ERR] AID=0x%04X dist=%ldmm st=%d", (unsigned int)aid, dist_mm, (int)st);
    }
#else
    (void)st;
#endif
#endif
}

static void tag_print_multi_anchor_summary(uint8_t n, const bool ok[], const float dist_m[])
{
    uint8_t ok_cnt = 0U;

    for (uint8_t i = 0U; i < n; i++) {
        if (ok[i]) ok_cnt++;
    }

    printf("[TAG][MULTI] %u/%u |",
           (unsigned int)ok_cnt,
           (unsigned int)n);

    for (uint8_t i = 0U; i < n; i++) {
        uint16_t aid = g_sys_cfg.anchor_ids[i];

        if (aid == 0U) {
            printf(" A%u[----]:SKIP", (unsigned int)i);
            continue;
        }

        if (!ok[i]) {
            printf(" A%u[%04X]:FAIL",
                   (unsigned int)i,
                   (unsigned int)aid);
            continue;
        }

        /* DS mode: TAG does not compute distance; show exchange success only */
        if (g_sys_cfg.ranging_mode == UWB_RANGING_MODE_DS) {
            printf(" A%u[%04X]:DS_OK",
                   (unsigned int)i,
                   (unsigned int)aid);
            continue;
        }

        /* SS mode: show measured distance */
        long dist_mm_l = 0L;
        if (dist_m[i] > 0.0f) {
            float mm_f = dist_m[i] * 1000.0f;
            if (mm_f > 2147483000.0f) mm_f = 2147483000.0f;
            dist_mm_l = (long)(mm_f + 0.5f);
        }

        printf(" A%u[%04X]:%ldmm",
               (unsigned int)i,
               (unsigned int)aid,
               dist_mm_l);

        uwb_can_send_anchor_distance_mm(aid, dist_mm_l);
    }

    printf("\r\n");
}

void tag_main(void) {
    printf("[TAG][APP] Starting TAG application (Target: %u Hz)\r\n", (unsigned)TAG_RANGING_RATE_HZ);
    
    g_tag_snap = g_tag_stat;
#if DS_RUNTIME_TRACK_CLEAN_CEILING
    printf("[RUNCLASS][START] track=CLEAN_CEILING hz=%u quiet=%u topo=1:1\r\n",
           (unsigned int)TAG_RANGING_RATE_HZ, (unsigned int)DS_RUNTIME_QUIET_MEASURE);
#endif

    while (1)
    {
        uint32_t t_now_us = micros();
        uint32_t t_start_us = t_now_us;
        
        uint8_t n = g_sys_cfg.anchor_count;
        if (n == 0U) n = 1U;
        if (n > 8U)  n = 8U;

        bool  ok[8];
        float dist_m[8];

        for (uint8_t i = 0; i < n; i++) {
            ok[i]     = false;
            dist_m[i] = 0.0f;
        }



#if DS_BOUNDED_SCHEDULER
        /* Phase 4B: Macro-gated Bounded Scheduler */
        uint32_t t_cycle_start = micros();
        // Build-time guard estimate only, not a hardware timing constant
        const uint32_t DS_BOUNDED_RETRY_COST_EST_US = 25000U; 
        const uint32_t period_target_us = 1000000U / TAG_RANGING_RATE_HZ;

        for (uint8_t i = 0; i < n; i++) {
            uint16_t aid = g_sys_cfg.anchor_ids[i];
            if (aid == 0U) {
                ok[i] = false;
                continue;
            }

            /* 1. Preserve Sequence Integrity (Guards must remain unconditional) */
            if (i == 1) {
                micros_delay(TAG_SLOT2_GUARD_US);
            } else if (i == 2) {
                micros_delay(TAG_SLOT3_GUARD_US);
            }

            uint32_t t_anchor_start = micros();
            uint32_t slot_envelope_us = TAG_ANCHOR_SLOT_MS * 1000U;

            /* 2. Bounded Polling Envelope */
            do {
                ok[i] = poll_one_aid(aid, &dist_m[i]);

                if (ok[i]) break; // Success

                /* Check budgets before attempting retry */
                uint32_t t_now = micros();
                uint32_t elapsed_in_slot = t_now - t_anchor_start;
                uint32_t elapsed_in_cycle = t_now - t_cycle_start;

                /* Per-anchor envelope check */
                if ((elapsed_in_slot + DS_BOUNDED_RETRY_COST_EST_US) > slot_envelope_us) {
                    break;
                }

                /* Global cycle budget check */
                uint32_t final_gap_reserve_us = (uint32_t)TAG_MIN_GAP_MS * 1000U;
                if ((elapsed_in_cycle + DS_BOUNDED_RETRY_COST_EST_US + final_gap_reserve_us) > period_target_us) {
                    break;
                }

                // Prepare for retry
                dw_recover_idle();
                
            } while (true);
        }
#else
        /* 1st pass: Poll all anchors defined in runtime config */
        for (uint8_t i = 0; i < n; i++) {
            uint16_t aid = g_sys_cfg.anchor_ids[i];
            if (aid == 0U) {
                ok[i] = false;
                continue;
            }

            if (i == 1) {
                micros_delay(TAG_SLOT2_GUARD_US);
            } else if (i == 2) {
                micros_delay(TAG_SLOT3_GUARD_US);
            }

            ok[i] = poll_one_aid(aid, &dist_m[i]);
        }
#endif

#if !DS_BOUNDED_SCHEDULER
        /* 2nd pass: Retry only failures once after recovery */
        for (uint8_t i = 0; i < n; i++) {
            if (ok[i]) continue;
            uint16_t aid = g_sys_cfg.anchor_ids[i];
            if (aid == 0U) continue;

            dw_recover_idle();
            ok[i] = poll_one_aid(aid, &dist_m[i]);
        }
#endif


#if !DS_RUNTIME_QUIET_MEASURE && (TAG_RANGING_RATE_HZ <= 64U)
        /* Report multi-anchor summary (A1..A3 typically) */
        tag_print_multi_anchor_summary(n, ok, dist_m);
#endif

        /* Stats aggregation */
        bool any_ok = false;
        for (uint8_t i = 0; i < n; i++) {
            if (ok[i]) {
                g_tag_stat.rx_ok++;
                any_ok = true;
            } else {
                g_tag_stat.fail_cnt++;
            }
        }

        g_tag_stat.total_cycles++;
        if (any_ok) {
            g_tag_stat.current_fail_burst = 0;
        } else {
            g_tag_stat.current_fail_burst++;
            if (g_tag_stat.current_fail_burst > g_tag_stat.longest_fail_burst) {
                g_tag_stat.longest_fail_burst = g_tag_stat.current_fail_burst;
            }
        }



        /* Scalable Pacing Logic: microsecond precision */
        {
            uint32_t t_active_us = (uint32_t)(micros() - t_start_us);
            uint32_t period_target_us = 1000000U / TAG_RANGING_RATE_HZ;
            uint32_t period_min_us    = n * TAG_ANCHOR_SLOT_MS * 1000U;
            
            /* Single-axis relaxation: Bypass slot-floor for N=1 DS success path to allow 64 Hz target */
            if (n == 1 && g_sys_cfg.ranging_mode == UWB_RANGING_MODE_DS && any_ok) {
                period_min_us = 0;
            }
            
            if (period_target_us < period_min_us) period_target_us = period_min_us;
            
            uint32_t gap_to_use_us = any_ok ? 1000U : (uint32_t)TAG_MIN_GAP_MS * 1000U;
            uint32_t wait_us = 0U;

            if (t_active_us + gap_to_use_us > period_target_us) {
                wait_us = gap_to_use_us;
            } else {
                wait_us = period_target_us - t_active_us;
            }
            micros_delay(wait_us);
            
#if UWB_RUNTIME_VERBOSE_LOGS
            bool overrun = (t_active_us + gap_to_use_us > period_target_us);
            uint32_t slack_us = overrun ? 0U : (period_target_us - t_active_us);
#if !DS_RUNTIME_QUIET_MEASURE && (TAG_RANGING_RATE_HZ <= 64U)
            printf("[TAG][CYCLE] air:%luus wait:%luus tgt:%luus ovr:%u slack:%luus\r\n", 
                   (unsigned long)t_active_us, (unsigned long)wait_us, (unsigned long)period_target_us, 
                   (unsigned int)(overrun ? 1U : 0U), (unsigned long)slack_us);
#endif
#endif
#if UWB_RUNTIME_VERBOSE_LOGS
#if DS_RUNTIME_INSTRUMENTATION
            printf("[TAG][BUDGET_DET] p1:%lu p2:%lu s1:%lu s2:%lu s3:%lu g2:%lu g3:%lu soft:%lu\r\n",
                   (unsigned long)(t_pass1_end - t_pass1_start),
                   (unsigned long)(t_pass2_end - t_pass2_start),
                   (unsigned long)t_slot_dur[0], (unsigned long)t_slot_dur[1], (unsigned long)t_slot_dur[2],
                   (unsigned long)t_guard_dur[1], (unsigned long)t_guard_dur[2],
                   (unsigned long)(t_soft_end - t_soft_start));
#endif
#endif

#if DS_RUNTIME_QUIET_MEASURE
            static uint32_t last_summary_ms = 0;
            static uint32_t last_ok_cumulative = 0;
            uint32_t t_now_tick = HAL_GetTick();
            uint32_t summary_cadence_ms = (TAG_RANGING_RATE_HZ >= 128U) ? 5000U : 1000U;
            if (t_now_tick - last_summary_ms >= summary_cadence_ms) {
                uint32_t run_ok = g_tag_stat.rx_ok - g_tag_snap.rx_ok;
                uint32_t run_fail = g_tag_stat.fail_cnt - g_tag_snap.fail_cnt;
                uint32_t run_total = run_ok + run_fail;

                uint32_t ok_delta = g_tag_stat.rx_ok - last_ok_cumulative;
                uint32_t ms_elapsed = t_now_tick - last_summary_ms;
                uint32_t obs_hz = (ms_elapsed > 0) ? (ok_delta * 1000U / ms_elapsed) : 0;

                uint32_t rate_100 = 0;
                if (run_total > 0) rate_100 = (run_ok * 10000U) / run_total;

                bool pass = (run_total > 0) && (rate_100 >= 9950U); // 99.5%
                bool pure = (g_tag_stat.rx_err == g_tag_snap.rx_err) && 
                            (g_tag_stat.rx_affrej == g_tag_snap.rx_affrej);

                printf("[TAG][QUIET] ok:%lu fail:%lu total:%lu rate:%lu.%02lu%% obs:%luHz target:%luHz\r\n",
                       (unsigned long)run_ok, (unsigned long)run_fail, (unsigned long)run_total,
                       (unsigned long)(rate_100 / 100), (unsigned long)(rate_100 % 100),
                       (unsigned long)obs_hz, (unsigned long)TAG_RANGING_RATE_HZ);

#if DS_RUNTIME_TRACK_CLEAN_CEILING
                printf("[RUNCLASS][STATUS] Result: %s | Purity: %s | success:%lu.%02lu%% | obs:%luHz | target:%luHz\r\n",
                       pass ? "PASS" : (run_total == 0 ? "PENDING" : "FAIL"),
                       pure ? "PURE" : "CONTAMINATED",
                       (unsigned long)(rate_100 / 100), (unsigned long)(rate_100 % 100),
                       (unsigned long)obs_hz, (unsigned long)TAG_RANGING_RATE_HZ);
#endif
                last_ok_cumulative = g_tag_stat.rx_ok;
                last_summary_ms = t_now_tick;
            }
#endif
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

static inline void hexdump(const char* tag, const uint8_t* p, uint32_t n)
{
    char buf[256];
    int pos = 0;
    pos += snprintf(&buf[pos], sizeof(buf) - (size_t)pos, "%s (%lu bytes): ", tag, (unsigned long)n);
    for (uint32_t i = 0; i < n && pos < (int)sizeof(buf) - 4; i++) {
        pos += snprintf(&buf[pos], sizeof(buf) - (size_t)pos, "%02X ", (unsigned int)p[i]);
    }
    if (pos < (int)sizeof(buf) - 2) {
        buf[pos++] = '\r'; buf[pos++] = '\n'; buf[pos] = '\0';
    }
    printf("%s", buf);
}

static inline uint16_t get_u16_le(const uint8_t *p) {
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

void anchor_main()
{
    printf("[RX][APP] Starting RX application\r\n");
    

    
    /* Link State Tracking */
    uint32_t last_matched_ms = HAL_GetTick();
    uint32_t last_lost_log_ms = 0;
    uint32_t last_pwr_log_ms = 0;
    uint32_t last_duty_ms = 0;
    bool is_listening = true;
    uint32_t current_state_entry_ms = last_matched_ms;
    anchor_runtime_state_t current_state = ANCHOR_STATE_ACTIVE;

    /* --- Pre-calculate loop-reusable DTU/H units for delayed TX timing --- */
    const uint32_t resp_tx_dly_dtu_h =
        (uint32_t)(((uint64_t)POLL_RX_TO_RESP_TX_DLY_UUS * UUS_TO_DWT_TIME) >> 8);

    const uint16_t resp_tx_antd_offset = (uint16_t)(TX_ANT_DLY + RESP_TS_ADJ_UNITS);

    g_anc_snap = g_anc_stat;
#if DS_RUNTIME_TRACK_CLEAN_CEILING
    printf("[RUNCLASS][START] track=CLEAN_CEILING hz=%u quiet=%u topo=1:1\r\n",
           (unsigned int)TAG_RANGING_RATE_HZ, (unsigned int)DS_RUNTIME_QUIET_MEASURE);
#endif
#if DS_ANCHOR_OBS_SUMMARY
    g_anc_obs_summary.last_print_ms = HAL_GetTick();
#endif

    while (1)
    {
#if DS_ANCHOR_OBS_SUMMARY
        anc_obs_summary_maybe_print(5000U);
#endif
        /* Silent epoch watchdog check */
        if (g_has_first_succ && !g_silent_epoch_dumped) {
            uint32_t now_tick = HAL_GetTick();
            if ((uint32_t)(now_tick - g_last_succ_tick) > ANC_SILENT_EPOCH_THRESHOLD_MS) {
                anc_silent_epoch_dump(now_tick - g_last_succ_tick);
                g_silent_epoch_dumped = true;
            }
        }

        /* RE-ARM RX: Clear status bits and re-enable RX if listening is permitted.
         * Soft slot-gating and duty-cycle logic will control the is_listening flag. */
        dwt_write32bitreg(SYS_STATUS_ID, ANC_SYS_STATUS_HYGIENE_BIT_MASK);

        if (is_listening) {
            dwt_rxenable(DWT_START_RX_IMMEDIATE);
        }

        /* --- Polling loop with Link-State Timeout Check --- */
        uint32_t status;
        do
        {
            status = dwt_read32bitreg(SYS_STATUS_ID);
            
            /* --- B3: Link State Management (Duty-Cycle Support) --- */
            uint32_t tick_ms = HAL_GetTick();
            switch (current_state) {
                case ANCHOR_STATE_ACTIVE:
                    if ((uint32_t)(tick_ms - last_matched_ms) >= ANCHOR_LINK_TIMEOUT_MS) {
                        printf("[ANC][LINK] lost: no matched poll for %lu ms\r\n", (unsigned long)ANCHOR_LINK_TIMEOUT_MS);
                        current_state = ANCHOR_STATE_LINK_LOST;
                        current_state_entry_ms = tick_ms;
                        last_lost_log_ms = tick_ms;
                        
                        /* Ensure RX is ON for wide recovery */
                        if (!is_listening) {
                            is_listening = true;
                            dwt_rxenable(DWT_START_RX_IMMEDIATE);
                        }
                    } else {
                        /* Restore b58a35e behavior: always listening when ACTIVE */
                        if (!is_listening) {
                            is_listening = true;
                            dwt_rxenable(DWT_START_RX_IMMEDIATE);
                        }
                    }
                    break;

                case ANCHOR_STATE_LINK_LOST:
                    /* Still-lost reminders */
                    if ((uint32_t)(tick_ms - last_lost_log_ms) >= ANCHOR_LOST_LOG_MS) {
                        uint32_t total_lost_ms = (uint32_t)(tick_ms - last_matched_ms);
                        printf("[ANC][LINK] lost: still no matched poll for %lu ms\r\n", (unsigned long)total_lost_ms);
                        last_lost_log_ms = tick_ms;
                    }
                    /* B3 Threshold Check: Transition to LOW_POWER_LISTEN */
                    if ((uint32_t)(tick_ms - current_state_entry_ms) >= ANCHOR_LP_TIMEOUT_MS) {
#if UWB_RUNTIME_VERBOSE_LOGS
                        printf("[ANC][PWR] enter low-power listen: no matched poll for %lu ms\r\n", (unsigned long)ANCHOR_LP_TIMEOUT_MS);
#endif
                        current_state = ANCHOR_STATE_LOW_POWER_LISTEN;
                        current_state_entry_ms = tick_ms;
                        last_duty_ms = tick_ms;
                        is_listening = true; /* Conservative: start with a Listen window */
                        /* We don't force radio Off here; the state machine will manage it in the next window */
                    }
                    break;

                case ANCHOR_STATE_LOW_POWER_LISTEN:
                    /* B3 (Corrected): Managed Duty Cycle */
                    if (is_listening) {
                        /* In listen window: check if it's time to sleep */
                        if ((uint32_t)(tick_ms - last_duty_ms) >= ANCHOR_LP_LISTEN_WINDOW_MS) {
                            is_listening = false;
                            last_duty_ms = tick_ms;
                            dwt_forcetrxoff(); /* Localized Radio OFF */
                        }
                    } else {
                        /* In sleep window: check if it's time to wake */
                        if ((uint32_t)(tick_ms - last_duty_ms) >= (ANCHOR_LP_WAKE_INTERVAL_MS - ANCHOR_LP_LISTEN_WINDOW_MS)) {
                            is_listening = true;
                            last_duty_ms = tick_ms;
                            dwt_rxenable(DWT_START_RX_IMMEDIATE); /* Localized Radio ON */
                            
                            /* Rate-limited wake log (every 10s) */
                            if ((uint32_t)(tick_ms - last_pwr_log_ms) >= 10000U) {
#if UWB_RUNTIME_VERBOSE_LOGS
                                printf("[ANC][PWR] wake: periodic check\r\n");
#endif
                                last_pwr_log_ms = tick_ms;
                            }
                        }
                    }
                    break;

                default:
                    /* State DEEP_SLEEP remains as behavior placeholders in B3 */
                    break;
            }

        } while (!(status & (SYS_STATUS_RXFCG_BIT_MASK | SYS_STATUS_AFFREJ_BIT_MASK | SYS_STATUS_RXERR_ALL_MASK | SYS_STATUS_RXTO_ALL_MASK)));

        /* [ERROR / TIMEOUT / FILTER-REJECT PATH] */
        if ((status & SYS_STATUS_RXFCG_BIT_MASK) == 0U)
        {
            uint32_t clr = 0U;
            const uint32_t arfe   = ((status & SYS_STATUS_ARFE_BIT_MASK) != 0U) ? 1U : 0U;
            const uint32_t affrej = ((status & SYS_STATUS_AFFREJ_BIT_MASK) != 0U) ? 1U : 0U;
            const uint32_t is_affrej = (affrej || arfe) ? 1U : 0U;
            const uint32_t is_to     = ((status & SYS_STATUS_RXTO_ALL_MASK)  != 0U) ? 1U : 0U;
            const uint32_t rxerr_no_arfe = (SYS_STATUS_RXERR_ALL_MASK & ~SYS_STATUS_ARFE_BIT_MASK);
            const uint32_t is_err    = ((status & rxerr_no_arfe) != 0U) ? 1U : 0U;

            if (is_affrej) { 
                g_anc_stat.rx_affrej++; 
#if DS_ANCHOR_OBS_SUMMARY
                g_anc_obs_summary.affrej_count++;
#endif
                clr |= SYS_STATUS_AFFREJ_BIT_MASK; 
                
                /* [NEW] REJ_DBG: Diagnostic snapshot of what the hardware rejected */
                uint32_t rej_finfo = dwt_read32bitreg(RX_FINFO_ID);
                uint32_t rej_flen = rej_finfo & RXFLEN_MASK;
                uint8_t rej_hdr[12] = {0};
                /* Only attempt read if frame info seems at least partially valid */
                if (rej_flen > 0 && rej_flen < 1024) {
                    dwt_readrxdata(rej_hdr, (rej_flen > 12) ? 12 : rej_flen, 0);
                }
#if UWB_RUNTIME_VERBOSE_LOGS
                printf("[TWR][ANC][REJ_DBG] AID=0x%04X flen=%lu HEX:", 
                       (unsigned int)g_sys_cfg.short_addr, (unsigned long)rej_flen);
                for (int i=0; i<12; i++) printf(" %02X", rej_hdr[i]);
                printf("\r\n");
#endif

#if defined(DS_TWR_VERBOSE) && DS_TWR_VERBOSE
                static uint32_t bg_affrej_cnt = 0;
                static uint32_t last_bg_affrej_print = 0;
                static uint32_t bg_last_ok = 0;
                static uint32_t bg_last_err = 0;
                static uint32_t bg_last_to = 0;
                static uint32_t bg_last_late = 0;
                
                bg_affrej_cnt++;
                uint32_t now_tick = HAL_GetTick();
                if ((uint32_t)(now_tick - last_bg_affrej_print) >= 1000U) {
                    uint32_t d_ok = g_anc_stat.rx_ok - bg_last_ok;
                    uint32_t d_err = g_anc_stat.rx_err - bg_last_err;
                    uint32_t d_to = g_anc_stat.rx_to - bg_last_to;
                    uint32_t d_late = g_anc_stat.tx_late - bg_last_late;
                    
                    printf("[ANC][BG] affrej=%lu active=%d listen=%d ok=%lu err=%lu to=%lu late=%lu\r\n",
                           (unsigned long)bg_affrej_cnt,
                           (current_state == ANCHOR_STATE_ACTIVE) ? 1 : 0,
                           (int)is_listening,
                           (unsigned long)d_ok,
                           (unsigned long)d_err,
                           (unsigned long)d_to,
                           (unsigned long)d_late);
                           
                    bg_affrej_cnt = 0;
                    last_bg_affrej_print = now_tick;
                    bg_last_ok = g_anc_stat.rx_ok;
                    bg_last_err = g_anc_stat.rx_err;
                    bg_last_to = g_anc_stat.rx_to;
                    bg_last_late = g_anc_stat.tx_late;
                }
#endif
            }
            if (is_to)     { 
                g_anc_stat.rx_to++;     
#if DS_ANCHOR_OBS_SUMMARY
                g_anc_obs_summary.rx_to_count++;
#endif
                clr |= SYS_STATUS_RXTO_ALL_MASK;
                anc_terminal_snapshot_record("TO", status);
            }
            if ((!is_affrej) && (!is_to) && is_err) { 
                g_anc_stat.rx_err++; 
#if DS_ANCHOR_OBS_SUMMARY
                g_anc_obs_summary.rx_err_count++;
#endif
                clr |= SYS_STATUS_RXERR_ALL_MASK;
                anc_terminal_snapshot_record("ERR", status);
            }
            if (is_affrej && is_err) { clr |= SYS_STATUS_RXERR_ALL_MASK; }

            /* Compact raw abnormal line for per-event tracing of non-matched frames */
            /* Filtered: Only REJ and ERR are logged to prevent RXTO flooding */
            if (!is_to) {
                const char* cls_str = is_affrej ? "REJ" : "ERR";
                anc_terminal_snapshot_record(cls_str, status);
#if UWB_RUNTIME_VERBOSE_LOGS
                printf("[TWR][ANC][RAW] AID=0x%04X cls=%s st=0x%08lX match=0\r\n",
                       (unsigned int)g_sys_cfg.short_addr, cls_str, (unsigned long)status);
#endif
            }

            /* AFFREJ burst protection: re-arm if too many filtered frames arrive too fast */
            {
                uint32_t now_ms = HAL_GetTick();
                if ((uint32_t)(now_ms - g_affrej_window_ms) > 50U) {
                    g_affrej_window_ms = now_ms;
                    g_affrej_window_cnt = 0U;
                }
                if (is_affrej) g_affrej_window_cnt++;
                if (g_affrej_window_cnt >= 12U) {
                    dwt_forcetrxoff();
                    dwt_write32bitreg(SYS_STATUS_ID, ANC_SYS_STATUS_HYGIENE_BIT_MASK);
                    dwt_rxenable(DWT_START_RX_IMMEDIATE);
                    g_affrej_window_cnt = 0U;
                }
            }
            dwt_write32bitreg(SYS_STATUS_ID, ANC_SYS_STATUS_HYGIENE_BIT_MASK);

            /* Diagnostic stats: suppressed in LOW_POWER_LISTEN to keep LP logs clean */
            if (current_state != ANCHOR_STATE_LOW_POWER_LISTEN) {
#if DS_RUNTIME_QUIET_MEASURE
                static uint32_t last_quiet_ms = 0;
                uint32_t now_tick = HAL_GetTick();
                if (now_tick - last_quiet_ms >= 1000U) {
                    uint32_t d_rx_ok = g_anc_stat.rx_ok - g_anc_snap.rx_ok;
                    uint32_t d_rx_err = g_anc_stat.rx_err - g_anc_snap.rx_err;
                    uint32_t d_rx_affrej = g_anc_stat.rx_affrej - g_anc_snap.rx_affrej;
                    uint32_t d_rx_to = g_anc_stat.rx_to - g_anc_snap.rx_to;
                    uint32_t d_tx_ok = g_anc_stat.tx_ok - g_anc_snap.tx_ok;
                    uint32_t d_tx_late = g_anc_stat.tx_late - g_anc_snap.tx_late;

                    printf("[ANC][QUIET] ok:%lu err:%lu affrej:%lu to:%lu txok:%lu late:%lu [DELTA]\r\n",
                           (unsigned long)d_rx_ok, (unsigned long)d_rx_err, (unsigned long)d_rx_affrej,
                           (unsigned long)d_rx_to, (unsigned long)d_tx_ok, (unsigned long)d_tx_late);

#if DS_RUNTIME_TRACK_CLEAN_CEILING
                    bool contaminated = (d_rx_affrej > 0U) || (d_rx_err > 0U);
                    bool failed = (d_tx_late > 0U);
                    const char* status = contaminated ? "INVALID_CONTAMINATED" : (failed ? "VALID_CLEAN_FAIL" : "VALID_CLEAN_PASS");
                    printf("[RUNCLASS][STATUS] %s\r\n", status);
#endif
                    last_quiet_ms = now_tick;
                    g_anc_snap = g_anc_stat; 
                }
#else
                twr_stats_maybe_print("[ANC]", &g_anc_stat, 
                                      (current_state == ANCHOR_STATE_ACTIVE) ? 1000U : 10000U);
#endif
            }
            continue;
        }

        /* [SUCCESS PATH: Frame received and passed hardware filtering] */
        dwt_write32bitreg(SYS_STATUS_ID, ANC_SYS_STATUS_HYGIENE_BIT_MASK);
        uint32_t flen = dwt_read32bitreg(RX_FINFO_ID) & RXFLEN_MASK;
        if (flen > sizeof(rx_buf) || flen < ALL_MSG_COMMON_LEN) continue;

        dwt_readrxdata(rx_buf, flen, 0);

        /* Verify standard frame control and UWB message type (0xE0 = POLL) */
        const bool fc_ok   = (rx_buf[0] == 0x41 && rx_buf[1] == 0x88);
        const bool type_ok = (flen > POLL_TYPE_IDX) && (rx_buf[POLL_TYPE_IDX] == 0xE0);
        if (!fc_ok || !type_ok) continue;

        const uint16_t rx_dst_pan   = get_u16_le(&rx_buf[POLL_DST_PAN_IDX]);
        const uint16_t rx_dst_addr  = get_u16_le(&rx_buf[POLL_DST_ADDR_IDX]);
        const uint16_t poll_src_addr = get_u16_le(&rx_buf[POLL_SRC_ADDR_IDX]);

        const uint8_t sn_rx = rx_buf[ALL_MSG_SN_IDX];
        const bool pan_match = (rx_dst_pan == _mac.panId);
        const bool addr_match = (rx_dst_addr == _mac.addr || rx_dst_addr == 0xFFFF);

        /* Software Filter: Ensure packet is for this board and network */
        if (!pan_match || !addr_match) {
            if (!pan_match)  { g_anc_stat.rx_fcg_bad_pan++; }
            if (!addr_match) { g_anc_stat.rx_fcg_bad_dst++; }
#if DS_ANCHOR_OBS_SUMMARY
            g_anc_obs_summary.rejected_nonmatching_poll_count++;
#endif
            
            /* Compact abnormal line for matched-type non-poll or filtered polls */
            /* If it was a poll (0xE0) but filtered, report it briefly for correlation */
            if (type_ok) {
#if UWB_RUNTIME_VERBOSE_LOGS
                printf("[TWR][ANC][ABN] SN=%u AID=0x%04X rx_ok=1 sch=0 tx_st=0 tx_ok=0 err=filt(%c%c)\r\n",
                       (unsigned int)sn_rx, (unsigned int)g_sys_cfg.short_addr,
                       pan_match ? '.' : 'P', addr_match ? '.' : 'A');
#endif
            }
            continue;
        }

        /* --- B3: Power/Link State Detection (Transition back to ACTIVE) --- */
        if (current_state != ANCHOR_STATE_ACTIVE) {
#if UWB_RUNTIME_VERBOSE_LOGS
            if (current_state == ANCHOR_STATE_LOW_POWER_LISTEN) {
                printf("[ANC][PWR] exit low-power listen: matched poll received\r\n");
            }
            printf("[ANC][LINK] resumed: matched poll received\r\n");
#endif
            current_state = ANCHOR_STATE_ACTIVE;
            current_state_entry_ms = HAL_GetTick();
            /* Cleanly reset duty-cycle internals for future re-entry */
            is_listening = true;
            last_duty_ms = HAL_GetTick();
        }
        last_matched_ms = HAL_GetTick();
        g_anc_stat.rx_ok++;
#if DS_ANCHOR_OBS_SUMMARY
        g_anc_obs_summary.matched_poll_count++;
#endif

        /* --- Delayed Response TX Logic --- */
        const uint64_t poll_rx_ts_40 = get_rx_timestamp_u64();
        const uint32_t poll_rx_ts_h  = (uint32_t)((poll_rx_ts_40 & 0xFFFFFFFFFFULL) >> 8);
        const uint32_t target_h      = poll_rx_ts_h + resp_tx_dly_dtu_h;
        dwt_setdelayedtrxtime(target_h);

        const uint64_t resp_tx_ts_40 = (((uint64_t)(target_h & 0xFFFFFFFEUL)) << 8) + (uint64_t)resp_tx_antd_offset;

        /* Construct RESP message from POLL source info */
        tx_resp_msg[ALL_MSG_SN_IDX] = rx_buf[ALL_MSG_SN_IDX];
        tx_resp_msg[RESP_TYPE_IDX]  = 0xE1;
        set_u16_le(&tx_resp_msg[RESP_DST_PAN_IDX],  _mac.panId);
        set_u16_le(&tx_resp_msg[RESP_DST_ADDR_IDX], poll_src_addr);
        set_u16_le(&tx_resp_msg[RESP_SRC_ADDR_IDX], _mac.addr);
        put_ts4(&tx_resp_msg[RESP_MSG_POLL_RX_TS_IDX],  poll_rx_ts_40);
        put_ts4(&tx_resp_msg[RESP_MSG_RESP_TX_TS_IDX], resp_tx_ts_40);

        dwt_writetxdata(sizeof(tx_resp_msg), tx_resp_msg, 0);
        dwt_writetxfctrl(sizeof(tx_resp_msg) + 2, 0, 1);
#if DS_ANCHOR_OBS_SUMMARY
        g_anc_obs_summary.response_tx_attempt_count++;
#endif
        
        if (dwt_starttx(DWT_START_TX_DELAYED) == DWT_ERROR) {
#if UWB_RUNTIME_VERBOSE_LOGS
            const uint32_t now_h  = dwt_readsystimestamphi32();
            const uint64_t now_40 = ((uint64_t)now_h) << 8;
#endif
#if UWB_RUNTIME_VERBOSE_LOGS
            int32_t late_us = 0;
            if (now_40 > resp_tx_ts_40) {
                uint64_t diff = now_40 - resp_tx_ts_40;
                if (diff > 0x7FFFFFFFULL) diff = 0x7FFFFFFFULL;
                late_us = (int32_t)(diff / UUS_TO_DWT_TIME);
            }
#endif
            g_anc_stat.tx_late++;
#if UWB_RUNTIME_VERBOSE_LOGS
            twr_stats_maybe_print("[ANC]", &g_anc_stat, 1000U);
#endif
#if UWB_RUNTIME_VERBOSE_LOGS
            /* Standard CORR line for sch failure */
            printf("[TWR][ANC][CORR] SN=%u AID=0x%04X rx_ok=1 sch=1 tx_ok=0 late_us=%ld\r\n",
                   (unsigned int)sn_rx,
                   (unsigned int)g_sys_cfg.short_addr,
                   (long)late_us);
            /* Supplemental abnormal line for full state capture */
            printf("[TWR][ANC][ABN] SN=%u AID=0x%04X rx_ok=1 sch=1 tx_st=0 tx_ok=0 err=tx_late(%ld)\r\n",
                   (unsigned int)sn_rx, (unsigned int)g_sys_cfg.short_addr, (long)late_us);
#endif
            continue;
        }

        /* Wait for transmit completion */
        uint32_t st_tx;
        do { st_tx = dwt_read32bitreg(SYS_STATUS_ID); } while ((st_tx & SYS_STATUS_TXFRS_BIT_MASK) == 0U);
        dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_TXFRS_BIT_MASK);

        g_anc_stat.tx_ok++;
#if DS_ANCHOR_OBS_SUMMARY
        g_anc_obs_summary.response_tx_ok_count++;
#endif
#if UWB_RUNTIME_VERBOSE_LOGS
        printf("[TWR][ANC][CORR] SN=%u AID=0x%04X rx_ok=1 sch=1 tx_ok=1\r\n",
               (unsigned int)sn_rx,
               (unsigned int)g_sys_cfg.short_addr);
#endif

        /* Check for sequence gap and dump miss context if needed */
        if (g_has_first_succ && (uint8_t)(sn_rx - g_last_succ_sn) > 3) {
            anc_terminal_snapshot_dump(g_last_succ_sn, sn_rx);
        }
        anc_terminal_snapshot_clear();
        g_last_succ_sn = sn_rx;
        g_last_succ_tick = HAL_GetTick();
        g_silent_epoch_dumped = false;
        g_has_first_succ = true;

        /* ============================================================= */
        /* DS-TWR branch: ANCHOR waits for FINAL, computes distance      */
        /* ============================================================= */
        if (g_sys_cfg.ranging_mode == UWB_RANGING_MODE_DS) {
            /* Save anchor-side timestamps for later DS calculation */
            const uint32_t anc_poll_rx_ts = (uint32_t)(poll_rx_ts_40 & 0xFFFFFFFFUL);
            const uint32_t anc_resp_tx_ts = (uint32_t)(resp_tx_ts_40 & 0xFFFFFFFFUL);

#if defined(DS_TWR_VERBOSE) && DS_TWR_VERBOSE
            static uint32_t dbg_final_affrej_burst = 0;
            static uint32_t dbg_final_rxerr_burst = 0;
            static uint32_t dbg_final_mism_burst = 0;
#endif

            /* Open second RX window for FINAL message */
            dwt_write32bitreg(SYS_STATUS_ID, ANC_SYS_STATUS_HYGIENE_BIT_MASK);
            dwt_setrxtimeout(FINAL_RX_TIMEOUT_UUS);
            dwt_rxenable(DWT_START_RX_IMMEDIATE);

            /* Wait for FINAL RX or timeout */
            uint32_t st_final;
            uint32_t watched_rx_mask = SYS_STATUS_RXFCG_BIT_MASK | SYS_STATUS_ALL_RX_ERR | SYS_STATUS_ALL_RX_TO | SYS_STATUS_AFFREJ_BIT_MASK;
            do {
                st_final = dwt_read32bitreg(SYS_STATUS_ID);
            } while (!(st_final & watched_rx_mask));

            /* Hygiene: Explicitly clear ALL watched RX events immediately. 
             * This prevents stale error/affrej bit residue from bleeding into the next cycle's evaluation. */
            dwt_write32bitreg(SYS_STATUS_ID, ANC_SYS_STATUS_HYGIENE_BIT_MASK);

            if (!(st_final & SYS_STATUS_RXFCG_BIT_MASK)) {
#if DS_ANCHOR_OBS_SUMMARY
                const bool has_rx_to = ((st_final & SYS_STATUS_ALL_RX_TO) != 0U);
                const bool has_rx_err = ((st_final & SYS_STATUS_ALL_RX_ERR) != 0U);
                const bool has_affrej = ((st_final & SYS_STATUS_AFFREJ_BIT_MASK) != 0U);

                if (has_rx_to && !has_rx_err && !has_affrej) {
                    g_anc_obs_summary.final_rx_timeout_count++;
                } else if (!has_rx_to && (has_rx_err || has_affrej)) {
                    g_anc_obs_summary.final_rx_err_count++;
                } else {
                    /* Composite/ambiguous no-RXFCG status (e.g. RX_TO+ERR/AFFREJ overlap or no explicit class bit). */
                    g_anc_obs_summary.final_rx_other_count++;
                }
#endif
                /* FINAL not received — diagnose reason */
#if defined(DS_TWR_VERBOSE) && DS_TWR_VERBOSE
                if (st_final & SYS_STATUS_AFFREJ_BIT_MASK) {
                    dbg_final_affrej_burst++;
                    printf("[ANC][DS] FINAL affrej (SN=%u) st=0x%08lX\r\n",
                           (unsigned)rx_buf[ALL_MSG_SN_IDX], (unsigned long)st_final);
                } else if (st_final & SYS_STATUS_ALL_RX_ERR) {
                    dbg_final_rxerr_burst++;
                    printf("[ANC][DS] FINAL rxerr (SN=%u) st=0x%08lX\r\n",
                           (unsigned)rx_buf[ALL_MSG_SN_IDX], (unsigned long)st_final);
                } else {
                    printf("[ANC][DS] FINAL timeout (SN=%u) st=0x%08lX\r\n",
                           (unsigned)rx_buf[ALL_MSG_SN_IDX], (unsigned long)st_final);
                }
#endif
                dwt_forcetrxoff(); /* Defensive recovery: ensure RX engine is fully idle before next cycle */
                continue;
            }

            /* Read and validate FINAL frame */
            uint32_t final_flen = dwt_read32bitreg(RX_FINFO_ID) & RXFLEN_MASK;
            if (final_flen > sizeof(rx_buf) || final_flen < FINAL_MSG_MIN_LEN) {
#if DS_ANCHOR_OBS_SUMMARY
                g_anc_obs_summary.final_rx_bad_len_count++;
#endif
#if defined(DS_TWR_VERBOSE) && DS_TWR_VERBOSE
                dbg_final_mism_burst++;
                printf("[ANC][DS] FINAL bad len (SN=%u)\r\n", (unsigned)rx_buf[ALL_MSG_SN_IDX]);
#endif
                dwt_forcetrxoff();
                continue;
            }

            uint8_t final_buf[64];
            dwt_readrxdata(final_buf, final_flen, 0);

            /* Validate: FC + FINAL type (0xE2) + SN match + PAN/addr match */
            bool final_fc_ok   = (final_buf[0] == 0x41 && final_buf[1] == 0x88);
            bool final_type_ok = (final_flen > FINAL_TYPE_IDX) && (final_buf[FINAL_TYPE_IDX] == 0xE2);
            bool final_sn_ok   = (final_buf[ALL_MSG_SN_IDX] == rx_buf[ALL_MSG_SN_IDX]);
            uint16_t final_dst_pan  = get_u16_le(&final_buf[FINAL_DST_PAN_IDX]);
            uint16_t final_dst_addr = get_u16_le(&final_buf[FINAL_DST_ADDR_IDX]);
            uint16_t final_src_addr = get_u16_le(&final_buf[FINAL_SRC_ADDR_IDX]);
            bool final_pan_ok  = (final_dst_pan == _mac.panId);
            bool final_dst_ok  = (final_dst_addr == _mac.addr);
            bool final_src_ok  = (final_src_addr == poll_src_addr);

            if (!final_fc_ok || !final_type_ok || !final_sn_ok || !final_pan_ok || !final_dst_ok || !final_src_ok) {
#if DS_ANCHOR_OBS_SUMMARY
                g_anc_obs_summary.final_rx_mismatch_count++;
#endif
#if defined(DS_TWR_VERBOSE) && DS_TWR_VERBOSE
                dbg_final_mism_burst++;
                printf("[ANC][DS] FINAL mismatch (SN=%u)\r\n", (unsigned)rx_buf[ALL_MSG_SN_IDX]);
#endif
                dwt_forcetrxoff();
                continue;
            }

            /* Read anchor-side final_rx timestamp */
            uint32_t anc_final_rx_ts = dwt_readrxtimestamplo32();
#if DS_ANCHOR_OBS_SUMMARY
            g_anc_obs_summary.final_rx_ok_count++;
#endif

#if defined(DS_TWR_VERBOSE) && DS_TWR_VERBOSE
            if ((dbg_final_affrej_burst + dbg_final_rxerr_burst + dbg_final_mism_burst) > 0) {
                printf("[ANC][DS] FINAL PATH RECOVERED | Preceding bursts -> affrej:%lu rxerr:%lu mism:%lu\r\n",
                       (unsigned long)dbg_final_affrej_burst,
                       (unsigned long)dbg_final_rxerr_burst,
                       (unsigned long)dbg_final_mism_burst);
                dbg_final_affrej_burst = 0;
                dbg_final_rxerr_burst = 0;
                dbg_final_mism_burst = 0;
            }
#endif

            /* Extract TAG-side timestamps from FINAL payload */
            uint32_t tag_poll_tx_ts, tag_resp_rx_ts, tag_final_tx_ts;
            get_ts4(&final_buf[FINAL_POLL_TX_TS_IDX], &tag_poll_tx_ts);
            get_ts4(&final_buf[FINAL_RESP_RX_TS_IDX], &tag_resp_rx_ts);
            get_ts4(&final_buf[FINAL_FINAL_TX_TS_IDX], &tag_final_tx_ts);

            /* DS-TWR distance calculation (asymmetric, 6-timestamp formula) */
            /* Ra = resp_rx_ts - poll_tx_ts  (TAG round 1)  */
            /* Rb = final_rx_ts - resp_tx_ts (ANCHOR round 2) */
            /* Da = resp_tx_ts - poll_rx_ts  (ANCHOR reply 1) */
            /* Db = final_tx_ts - resp_rx_ts (TAG reply 2) */
            int32_t Ra = (int32_t)(tag_resp_rx_ts  - tag_poll_tx_ts);
            int32_t Rb = (int32_t)(anc_final_rx_ts - anc_resp_tx_ts);
            int32_t Da = (int32_t)(anc_resp_tx_ts  - anc_poll_rx_ts);
            int32_t Db = (int32_t)(tag_final_tx_ts - tag_resp_rx_ts);

#if defined(DS_TWR_VERBOSE) && DS_TWR_VERBOSE
            printf("[ANC][DS] Ra=%ld Rb=%ld Da=%ld Db=%ld\r\n",
                   (long)Ra, (long)Rb, (long)Da, (long)Db);
#endif

            double tof = (((double)Ra * (double)Rb) - ((double)Da * (double)Db)) /
                         ((double)Ra + (double)Rb + (double)Da + (double)Db) * DWT_TIME_UNITS;
            double ds_dist = tof * SPEED_OF_LIGHT;
            if (ds_dist < 0.0) ds_dist = 0.0;

            if (ds_dist > 0.0) {
                double d_mm = ds_dist * 1000.0;
                if (d_mm > 2147483647.0) d_mm = 2147483647.0;
                long ds_dist_mm = (long)d_mm;

                uint32_t can_id = 0x200U + (g_sys_cfg.short_addr & 0x0FU);
                uwb_can_send_anchor_distance_mm(g_sys_cfg.short_addr, ds_dist_mm);
                
                uint16_t final_src = get_u16_le(&final_buf[FINAL_SRC_ADDR_IDX]);
                printf("[ANC][DS][DIST] AID=0x%04X src=0x%04X dist=%ldmm can_id=0x%lX\r\n",
                       (unsigned int)g_sys_cfg.short_addr, (unsigned int)final_src, ds_dist_mm, (unsigned long)can_id);
            }
        } else {
            /* SS-TWR: no further action after RESP TX */
#if UWB_RUNTIME_VERBOSE_LOGS
#if defined(DS_TWR_VERBOSE) && DS_TWR_VERBOSE
            printf("[ANC][RESP][CORR] SN=%u src=0x%04X sched=1 txdone=1\r\n",
                   (unsigned int)rx_buf[ALL_MSG_SN_IDX],
                   (unsigned int)poll_src_addr);
#endif
#endif
        }
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
  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* Configure the system clock */
  SystemClock_Config();

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_SPI1_Init();
  MX_LPUART1_UART_Init();
  MX_FDCAN1_Init();
  can_start();
  MX_UART5_Init();
  MX_FMAC_Init();
  MX_TIM2_Init();


  HAL_Delay(1000);

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

#if UWB_ONE_WAY_BENCHMARK
    if (runtime_role == UWB_ROLE_TAG) {
        printf("[MAIN] Starting one-way benchmark TAG receiver\r\n");
        oneway_benchmark_tag_main();
    } else {
        printf("[MAIN] Starting one-way benchmark ANCHOR driver\r\n");
        oneway_benchmark_anchor_main();
    }
#else
    if (runtime_role == UWB_ROLE_TAG) {
        printf("[MAIN] Starting TX application (TAG)\r\n");
        tag_main();
    } else {
        printf("[MAIN] Starting RX application (ANCHOR)\r\n");
        anchor_main();
    }
#endif

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
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
    Error_Handler();
    }

    /** Initializes the CPU, AHB and APB buses clocks
     */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
    RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK) {
    Error_Handler();
    }
}

/* USER CODE BEGIN 4 */
int _write(int file, char *ptr, int len)
{
    (void)file;
    HAL_UART_Transmit(&hlpuart1, (uint8_t *)ptr, (uint16_t)len, HAL_MAX_DELAY);
    return len;
}
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
