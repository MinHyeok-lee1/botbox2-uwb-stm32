#!/usr/bin/env bash
set -euo pipefail

show_help() {
  cat <<'EOF'
Usage:
  ./tools/provision.sh [--fw firmware.bin] [--cfg cfg.bin] --sn <STLINK_SN>
                       [--freq <kHz>]

Modes:
  --fw + --cfg   : Full provisioning (firmware + config)
  --fw only      : Firmware update (preserves existing cfg)
  --cfg only     : Config update (preserves firmware)

Examples:
  # New device: firmware + config
  ./tools/provision.sh --fw firmware.bin --cfg configs/tag_default.bin --sn 004D003D3234510F37333934

  # Role change: config only
  ./tools/provision.sh --cfg configs/anchor_default.bin --sn 004D003D3234510F37333934

  # Firmware update: firmware only (cfg preserved)
  ./tools/provision.sh --fw firmware.bin --sn 004D003D3234510F37333934

Safety:
  - Mass erase is never performed
  - fw write uses BIN address mode (0x08000000), CFG_FLASH preserved
  - cfg write erases only Bank 2 Page 127 (0x0807F800)
  - fw .bin size > 510K (0x7F800) is rejected
  - cfg .bin size != 40 bytes is rejected
EOF
}

if [[ "${1:-}" == "--help" || "${1:-}" == "-h" ]]; then
  show_help
  exit 0
fi

CLI="${STM32_CLI:-STM32_Programmer_CLI}"
FREQ="${PROG_FREQ:-4000}"

CFG_FLASH_ADDR="0x0807F800"
FW_FLASH_ADDR="0x08000000"
MAX_FW_SIZE=$((510 * 1024))  # 0x7F800
EXPECTED_CFG_SIZE=40

log(){ echo -e "\033[1;36m==>\033[0m $*"; }
die(){ echo -e "\033[1;31m[ERR]\033[0m $*" >&2; exit 1; }

FW=""
CFG=""
SN=""

while [[ $# -gt 0 ]]; do
  case "$1" in
    --fw)  FW="$2";  shift 2 ;;
    --cfg) CFG="$2"; shift 2 ;;
    --sn)  SN="$2";  shift 2 ;;
    --freq) FREQ="$2"; shift 2 ;;
    *) die "unknown option: $1" ;;
  esac
done

command -v "$CLI" >/dev/null 2>&1 || die "STM32_Programmer_CLI not found"
[[ -n "$SN" ]] || die "--sn <STLINK_SN> required"
[[ -n "$FW" || -n "$CFG" ]] || die "at least one of --fw or --cfg required"

# Validate firmware file
if [[ -n "$FW" ]]; then
  [[ -f "$FW" ]] || die "firmware file not found: $FW"
  FW_SIZE=$(stat -c%s "$FW" 2>/dev/null || stat -f%z "$FW")
  if (( FW_SIZE > MAX_FW_SIZE )); then
    die "firmware size ($FW_SIZE bytes) exceeds 510K limit. CFG_FLASH would be overwritten. Aborting."
  fi
  log "Firmware : $FW ($FW_SIZE bytes)"
fi

# Validate cfg file
if [[ -n "$CFG" ]]; then
  [[ -f "$CFG" ]] || die "cfg file not found: $CFG"
  CFG_SIZE=$(stat -c%s "$CFG" 2>/dev/null || stat -f%z "$CFG")
  if (( CFG_SIZE != EXPECTED_CFG_SIZE )); then
    die "cfg size ($CFG_SIZE bytes) != expected $EXPECTED_CFG_SIZE bytes. Invalid cfg.bin."
  fi
  log "Config   : $CFG ($CFG_SIZE bytes)"
fi

log "ST-Link  : $SN"
log "SWD freq : $FREQ kHz"

# Step 1: Write firmware (if provided)
if [[ -n "$FW" ]]; then
  log "Writing firmware to $FW_FLASH_ADDR ..."
  "$CLI" \
    -c port=SWD freq="$FREQ" sn="$SN" \
    -w "$FW" "$FW_FLASH_ADDR" \
    -v
  log "Firmware write complete"
fi

# Step 2: Write config (if provided)
if [[ -n "$CFG" ]]; then
  log "Writing config to $CFG_FLASH_ADDR ..."
  "$CLI" \
    -c port=SWD freq="$FREQ" sn="$SN" \
    -w "$CFG" "$CFG_FLASH_ADDR" \
    -v
  log "Config write complete"
fi

# Step 3: Reset
log "Resetting MCU ..."
"$CLI" -c port=SWD freq="$FREQ" sn="$SN" -rst

log "Provisioning done"
