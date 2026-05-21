#include "uwb_cfg.h"
#include <string.h>
#include <stdio.h>
#include <stddef.h>
#include "stm32g4xx_hal.h"

#if defined(UWB_USE_USER_CFG_FILE)
#include "uwb_user_cfg.h"
#endif

// Define the Flash page to reserve for config
// G474RE has 512KB. Bank 1: 0x08000000-0x0803FFFF, Bank 2: 0x08040000-0x0807FFFF
// We reserve the very last page: Bank 2, Page 127
// Address: 0x08080000 - 0x800 = 0x0807F800
#define UWB_CFG_FLASH_START_ADDR 0x0807F800

// Software CRC32 function (standard IEEE 802.3 poly 0x04C11DB7, reflection/xorout may vary but keeping it simple)
static uint32_t calc_crc32(const uint8_t *data, size_t length) {
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < length; i++) {
        crc ^= ((uint32_t)data[i]) << 24;
        for (int j = 0; j < 8; j++) {
            if (crc & 0x80000000) {
                crc = (crc << 1) ^ 0x04C11DB7;
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}

// Internal validation function
bool uwb_cfg_is_valid(const uwb_cfg_t *cfg) {
    if (cfg == NULL) return false;
    if (cfg->magic != UWB_CFG_MAGIC) return false;
    if (cfg->version != UWB_CFG_VERSION) return false;
    if (cfg->length != sizeof(uwb_cfg_t)) return false;

    // Range checks for structural integrity and safe array access
    if (cfg->role > UWB_ROLE_TAG) return false;
    if (cfg->ranging_mode > UWB_RANGING_MODE_DS) return false;
    if (cfg->anchor_count > 8) return false; // Hard array limit
    if (cfg->filter_profile > FILTER_PROFILE_STRONG) return false;
    if (cfg->comm_mode > COMM_MODE_MULTICAST) return false;

    // Calculate CRC32 of everything except the crc32 field itself
    size_t crc_len = offsetof(uwb_cfg_t, crc32);
    if (crc_len > sizeof(uwb_cfg_t)) return false; // Sanity check

    uint32_t calc = calc_crc32((const uint8_t *)cfg, crc_len);
    return (calc == cfg->crc32);
}

/*
 * Pure Static Logic Validation for UWB Configuration
 * Validates fields for range/logic errors to prevent incorrect presets/configs
 * from entering runtime or persisting to Flash. Hardware access is not performed.
 */
bool uwb_cfg_validate_logic(const uwb_cfg_t *cfg) {
    if (cfg == NULL) {
        printf("[CFG][VAL] Error: NULL configuration pointer\r\n");
        return false;
    }
    if (cfg->magic != UWB_CFG_MAGIC) {
        printf("[CFG][VAL] Error: Invalid magic number 0x%08lX (expected 0x%08lX)\r\n",
               (unsigned long)cfg->magic, (unsigned long)UWB_CFG_MAGIC);
        return false;
    }
    if (cfg->version != UWB_CFG_VERSION) {
        printf("[CFG][VAL] Error: Unsupported version %u (expected %u)\r\n",
               (unsigned int)cfg->version, (unsigned int)UWB_CFG_VERSION);
        return false;
    }
    if (cfg->length != sizeof(uwb_cfg_t)) {
        printf("[CFG][VAL] Error: Length mismatch %u (expected %u)\r\n",
               (unsigned int)cfg->length, (unsigned int)sizeof(uwb_cfg_t));
        return false;
    }
    if (cfg->role > UWB_ROLE_TAG) {
        printf("[CFG][VAL] Error: Invalid role %u\r\n", (unsigned int)cfg->role);
        return false;
    }
    if (cfg->ranging_mode > UWB_RANGING_MODE_DS) {
        printf("[CFG][VAL] Error: Invalid ranging mode %u\r\n", (unsigned int)cfg->ranging_mode);
        return false;
    }
    if (cfg->anchor_count == 0 || cfg->anchor_count > 8) {
        printf("[CFG][VAL] Error: Anchor count %u out of bounds (1-8)\r\n", (unsigned int)cfg->anchor_count);
        return false;
    }
    if (cfg->filter_profile > FILTER_PROFILE_STRONG) {
        printf("[CFG][VAL] Error: Invalid filter profile %u\r\n", (unsigned int)cfg->filter_profile);
        return false;
    }
    if (cfg->comm_mode > COMM_MODE_MULTICAST) {
        printf("[CFG][VAL] Error: Invalid communication mode %u\r\n", (unsigned int)cfg->comm_mode);
        return false;
    }
    if (cfg->pan_id == 0x0000 || cfg->pan_id == 0xFFFF) {
        printf("[CFG][VAL] Error: Reserved/Invalid PAN ID 0x%04X\r\n", (unsigned int)cfg->pan_id);
        return false;
    }
    if (cfg->short_addr == 0x0000 || cfg->short_addr == 0xFFFF) {
        printf("[CFG][VAL] Error: Reserved/Invalid Short Address 0x%04X\r\n", (unsigned int)cfg->short_addr);
        return false;
    }

    // Anchor IDs verification
    for (uint8_t i = 0; i < cfg->anchor_count; i++) {
        uint16_t aid = cfg->anchor_ids[i];
        if (aid == 0x0000 || aid == 0xFFFF) {
            printf("[CFG][VAL] Error: Invalid Anchor ID 0x%04X at index %u\r\n", (unsigned int)aid, (unsigned int)i);
            return false;
        }

        // Self-polling check for TAG
        if (cfg->role == UWB_ROLE_TAG && aid == cfg->short_addr) {
            printf("[CFG][VAL] Error: Self-polling detected. TAG Short Address 0x%04X found in anchor_ids[%u]\r\n",
                   (unsigned int)cfg->short_addr, (unsigned int)i);
            return false;
        }

        // Duplicate anchor check
        for (uint8_t j = i + 1; j < cfg->anchor_count; j++) {
            if (aid == cfg->anchor_ids[j]) {
                printf("[CFG][VAL] Error: Duplicate Anchor ID 0x%04X detected at indices %u and %u\r\n",
                       (unsigned int)aid, (unsigned int)i, (unsigned int)j);
                return false;
            }
        }
    }

    // CRC validation
    size_t crc_len = offsetof(uwb_cfg_t, crc32);
    uint32_t calc = calc_crc32((const uint8_t *)cfg, crc_len);
    if (calc != cfg->crc32) {
        printf("[CFG][VAL] Error: CRC32 mismatch. Calculated 0x%08lX, got 0x%08lX\r\n",
               (unsigned long)calc, (unsigned long)cfg->crc32);
        return false;
    }

    return true;
}

void uwb_cfg_get_default(uwb_cfg_t *out_cfg, uint8_t role) {
    if (!out_cfg) return;
    
    // Clear everything first
    memset(out_cfg, 0, sizeof(uwb_cfg_t));

    out_cfg->magic = UWB_CFG_MAGIC;
    out_cfg->version = UWB_CFG_VERSION;
    out_cfg->length = sizeof(uwb_cfg_t);
    
    out_cfg->role = role; 
    
    // Hard defaults from legacy main.c overrides
#if defined(UWB_USE_USER_CFG_FILE)
    out_cfg->ranging_mode = UWB_CFG_FILE_RANGING_MODE;
    out_cfg->pan_id = UWB_CFG_FILE_PAN_ID;
    out_cfg->anchor_count = UWB_CFG_FILE_ANCHOR_COUNT;
    out_cfg->filter_profile = UWB_CFG_FILE_FILTER_PROFILE;
    out_cfg->comm_mode = UWB_CFG_FILE_COMM_MODE;
    if (role == UWB_ROLE_TAG) {
        out_cfg->short_addr = UWB_CFG_FILE_TAG_SHORT_ADDR;
    } else {
        out_cfg->short_addr = UWB_CFG_FILE_ANCHOR_SHORT_ADDR;
    }
    for (int i = 0; i < 8; i++) {
        // Safe sequential default
        out_cfg->anchor_ids[i] = UWB_CFG_FILE_ANCHOR_BASE_ADDR + i;
    }
    
    // Explicit overrides
#ifdef UWB_CFG_FILE_ANCHOR_0
    out_cfg->anchor_ids[0] = UWB_CFG_FILE_ANCHOR_0;
#endif
#ifdef UWB_CFG_FILE_ANCHOR_1
    out_cfg->anchor_ids[1] = UWB_CFG_FILE_ANCHOR_1;
#endif
#ifdef UWB_CFG_FILE_ANCHOR_2
    out_cfg->anchor_ids[2] = UWB_CFG_FILE_ANCHOR_2;
#endif

#else
    // Shared defaults from uwb_cfg.h
    out_cfg->ranging_mode   = UWB_DEFAULT_RANGING_MODE;
    out_cfg->pan_id         = UWB_DEFAULT_PAN_ID;
    out_cfg->anchor_count   = UWB_DEFAULT_ANCHOR_COUNT;
    out_cfg->filter_profile = UWB_DEFAULT_FILTER_PROFILE;
    out_cfg->comm_mode     = UWB_DEFAULT_COMM_MODE;
    
    if (role == UWB_ROLE_TAG) {
        out_cfg->short_addr = UWB_DEFAULT_TAG_SHORT_ADDR;
    } else {
        out_cfg->short_addr = UWB_DEFAULT_ANCHOR_SHORT_ADDR;
    }

    // Initialize all 8 slots based on base address
    for (int i = 0; i < 8; i++) {
        out_cfg->anchor_ids[i] = (uint16_t)(UWB_DEFAULT_ANCHOR_BASE_ADDR + i);
    }
#endif

    // Calculate CRC
    size_t crc_len = offsetof(uwb_cfg_t, crc32);
    out_cfg->crc32 = calc_crc32((const uint8_t *)out_cfg, crc_len);
}

bool uwb_cfg_load(uwb_cfg_t *out_cfg) {
    if (!out_cfg) return false;

    // Direct memory pointer to flash address
    const uwb_cfg_t *flash_cfg = (const uwb_cfg_t *)UWB_CFG_FLASH_START_ADDR;

    // Copy to RAM first to avoid misalignment issues on some ARMs, though G4 is usually okay
    memcpy(out_cfg, flash_cfg, sizeof(uwb_cfg_t));

    if (uwb_cfg_is_valid(out_cfg) && uwb_cfg_validate_logic(out_cfg)) {
        return true;
    }

    return false;
}

bool uwb_cfg_save(const uwb_cfg_t *cfg) {
    if (!cfg || !uwb_cfg_is_valid(cfg) || !uwb_cfg_validate_logic(cfg)) return false;

    HAL_FLASH_Unlock();

    // 1. Erase the page
    FLASH_EraseInitTypeDef erase_init;
    uint32_t page_error = 0;

    erase_init.TypeErase = FLASH_TYPEERASE_PAGES;
    erase_init.Banks = FLASH_BANK_2;
    erase_init.Page = 127; // Bank 2, Page 127
    erase_init.NbPages = 1;

    if (HAL_FLASHEx_Erase(&erase_init, &page_error) != HAL_OK) {
        HAL_FLASH_Lock();
        return false;
    }

    // 2. Program the page with 64-bit doubled words (STM32G4 Flash programs in Double-Words)
    const uint64_t *cfg_data_64 = (const uint64_t *)cfg;
    uint32_t dword_count = (sizeof(uwb_cfg_t) + 7) / 8; // round up
    
    // G474 requires Fast Program or Double-Word Program. Let's use Double-Word.
    uint32_t dest_addr = UWB_CFG_FLASH_START_ADDR;
    for (uint32_t i = 0; i < dword_count; i++) {
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, dest_addr, cfg_data_64[i]) != HAL_OK) {
            HAL_FLASH_Lock();
            return false;
        }
        dest_addr += 8;
    }

    HAL_FLASH_Lock();

    // Verify it saved correctly
    uwb_cfg_t verify_cfg;
    return uwb_cfg_load(&verify_cfg);
}

void uwb_cfg_factory_reset(uint8_t compiled_role) {
    uwb_cfg_t def;
    uwb_cfg_get_default(&def, compiled_role);
    uwb_cfg_save(&def);
}

void uwb_cfg_print(const uwb_cfg_t *cfg) {
    if (!cfg) return;

    printf("\r\n--- UWB CONFIGURATION ---\r\n");
    printf("Magic:      0x%08lX %s\r\n", (unsigned long)cfg->magic, (cfg->magic == UWB_CFG_MAGIC) ? "[OK]" : "[FAIL]");
    printf("Version:    %u\r\n", (unsigned int)cfg->version);
    printf("Length:     %u\r\n", (unsigned int)cfg->length);
    printf("CRC32:      0x%08lX %s\r\n", (unsigned long)cfg->crc32, uwb_cfg_is_valid(cfg) ? "[VALID]" : "[INVALID]");
    printf("Role:       %s\r\n", (cfg->role == UWB_ROLE_TAG) ? "TAG" : "ANCHOR");
    printf("PAN ID:     0x%04X\r\n", (unsigned int)cfg->pan_id);
    printf("Short Addr: 0x%04X\r\n", (unsigned int)cfg->short_addr);
    printf("Mode (SS/DS): %u\r\n", (unsigned int)cfg->ranging_mode);
    printf("Filter Pro: %u\r\n", (unsigned int)cfg->filter_profile);
    printf("Comm Mode:  %u\r\n", (unsigned int)cfg->comm_mode);
    printf("Anchors:    %u configured\r\n", (unsigned int)cfg->anchor_count);
    
    for (int i = 0; i < cfg->anchor_count && i < 8; i++) {
        printf("  AID[%d]:   0x%04X\r\n", (int)i, (unsigned int)cfg->anchor_ids[i]);
    }
    printf("-------------------------\r\n");
}

void uwb_cfg_update_crc(uwb_cfg_t *cfg) {
    if (!cfg) return;
    size_t crc_len = offsetof(uwb_cfg_t, crc32);
    cfg->crc32 = calc_crc32((const uint8_t *)cfg, crc_len);
}
