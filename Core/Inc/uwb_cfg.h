#ifndef UWB_CFG_H
#define UWB_CFG_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* -------------------------------------------------------------------------- */
/* UWB Default Configuration Macros                                           */
/* -------------------------------------------------------------------------- */
#ifndef UWB_DEFAULT_PAN_ID
#define UWB_DEFAULT_PAN_ID              0x2001
#endif

#ifndef UWB_DEFAULT_TAG_SHORT_ADDR
#define UWB_DEFAULT_TAG_SHORT_ADDR      0x1001
#endif

#ifndef UWB_DEFAULT_ANCHOR_BASE_ADDR
#define UWB_DEFAULT_ANCHOR_BASE_ADDR    0x2001
#endif

#ifndef UWB_DEFAULT_ANCHOR_SHORT_ADDR
#define UWB_DEFAULT_ANCHOR_SHORT_ADDR   0x2003 // Generic fallback
#endif

#ifndef UWB_DEFAULT_ANCHOR_COUNT
#define UWB_DEFAULT_ANCHOR_COUNT        3U
#endif

// Defaults for anchor slots
#define UWB_DEFAULT_ANCHOR_0   (UWB_DEFAULT_ANCHOR_BASE_ADDR + 0U)
#define UWB_DEFAULT_ANCHOR_1   (UWB_DEFAULT_ANCHOR_BASE_ADDR + 1U)
#define UWB_DEFAULT_ANCHOR_2   (UWB_DEFAULT_ANCHOR_BASE_ADDR + 2U)

#ifndef UWB_DEFAULT_RANGING_MODE
#define UWB_DEFAULT_RANGING_MODE        0 // UWB_RANGING_MODE_SS
#endif

#ifndef UWB_DEFAULT_FILTER_PROFILE
#define UWB_DEFAULT_FILTER_PROFILE      1 // FILTER_PROFILE_NORMAL
#endif

#ifndef UWB_DEFAULT_COMM_MODE
#define UWB_DEFAULT_COMM_MODE           0 // COMM_MODE_ANYCAST
#endif

#define UWB_CFG_MAGIC 0x55574243 // "UWBC"
#define UWB_CFG_VERSION 1

// Role indicator: 0 = ANCHOR (RX), 1 = TAG (TX)
#define UWB_ROLE_ANCHOR 0
#define UWB_ROLE_TAG    1

#define UWB_RANGING_MODE_SS  0
#define UWB_RANGING_MODE_DS  1

#define FILTER_PROFILE_WEAK   0
#define FILTER_PROFILE_NORMAL 1
#define FILTER_PROFILE_STRONG 2

#define COMM_MODE_ANYCAST     0
#define COMM_MODE_MULTICAST   1

typedef struct __attribute__((packed, aligned(4))) {
    uint32_t magic;         // 0x55574243
    uint16_t version;       // 1
    uint16_t length;        // sizeof(uwb_cfg_t)
    
    uint8_t  role;          // TAG(1) or ANCHOR(0)
    uint8_t  ranging_mode;  // 0: SS-TWR, 1: DS-TWR
    uint16_t pan_id;        // e.g., 0x2001
    
    uint16_t short_addr;    // Self address
    uint8_t  anchor_count;  // Max 8
    uint8_t  filter_profile;// 0: Weak, 1: Normal, 2: Strong
    
    uint8_t  comm_mode;     // 0: Anycast, 1: Multicast
    uint8_t  reserved[3];   // Padding for 4-byte alignment
    
    uint16_t anchor_ids[8]; // Array of Anchor IDs to poll (for TAG)
    
    uint32_t crc32;         // Integrity check via software CRC
} uwb_cfg_t;

// Public API
bool uwb_cfg_load(uwb_cfg_t *out_cfg);
bool uwb_cfg_save(const uwb_cfg_t *cfg);
void uwb_cfg_get_default(uwb_cfg_t *out_cfg, uint8_t compiled_role);
bool uwb_cfg_is_valid(const uwb_cfg_t *cfg);
void uwb_cfg_factory_reset(uint8_t compiled_role);
void uwb_cfg_print(const uwb_cfg_t *cfg);
void uwb_cfg_update_crc(uwb_cfg_t *cfg);

#endif // UWB_CFG_H
