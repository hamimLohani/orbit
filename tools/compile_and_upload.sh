#!/bin/bash
# ==============================================================================
# ORBIT v2 — Wi-Fi Compilation & OTA Upload Tool
# Usage:
#   ./tools/compile_and_upload.sh                  # Compile & upload to orbit.local
#   ./tools/compile_and_upload.sh 192.168.1.150    # Compile & upload to specific IP
#   ./tools/compile_and_upload.sh --bin <file.bin> # Upload pre-compiled binary
# ==============================================================================

set -e

CYAN='\033[0;36m'
MINT='\033[0;32m'
AMBER='\033[0;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

echo -e "${CYAN}=================================================================${NC}"
echo -e "${CYAN}   ORBIT v2 — Wi-Fi Firmware Compilation & OTA Uploader   ${NC}"
echo -e "${CYAN}=================================================================${NC}"

TARGET_IP="orbit.local"
PRECOMPILED_BIN=""

# Parse arguments
while [[ $# -gt 0 ]]; do
  case $1 in
    --bin)
      PRECOMPILED_BIN="$2"
      shift 2
      ;;
    *)
      TARGET_IP="$1"
      shift
      ;;
  esac
done

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"
BIN_FILE=""

mkdir -p "${BUILD_DIR}"

if [[ -n "${PRECOMPILED_BIN}" ]]; then
  if [[ ! -f "${PRECOMPILED_BIN}" ]]; then
    echo -e "${RED}[ERROR] Binary file not found: ${PRECOMPILED_BIN}${NC}"
    exit 1
  fi
  BIN_FILE="${PRECOMPILED_BIN}"
  echo -e "${MINT}[OK] Using pre-compiled binary: ${BIN_FILE}${NC}"
else
  # Compile Firmware
  echo -e "\n${AMBER}[1/2] Compiling ORBIT v2 Firmware...${NC}"

  # Check compilation tools
  ARDUINO_CLI=""
  if command -v arduino-cli &> /dev/null; then
    ARDUINO_CLI="arduino-cli"
  elif [[ -x "/opt/homebrew/bin/arduino-cli" ]]; then
    ARDUINO_CLI="/opt/homebrew/bin/arduino-cli"
  fi

  if [[ -n "${ARDUINO_CLI}" ]]; then
    echo -e "${CYAN}Found Arduino CLI at: ${ARDUINO_CLI}${NC}"
    FQBN="esp32:esp32:esp32s3"
    
    "${ARDUINO_CLI}" compile \
      --fqbn "${FQBN}" \
      --output-dir "${BUILD_DIR}" \
      --libraries "${ROOT_DIR}/src" \
      "${ROOT_DIR}/orbit.ino"

    BIN_FILE="${BUILD_DIR}/orbit.ino.bin"
  elif command -v pio &> /dev/null; then
    echo -e "${CYAN}Found PlatformIO. Building with pio run...${NC}"
    cd "${ROOT_DIR}"
    pio run -e esp32-s3-devkitc-1
    BIN_FILE="${ROOT_DIR}/.pio/build/esp32-s3-devkitc-1/firmware.bin"
  else
    echo -e "${RED}[ERROR] Neither 'arduino-cli' nor 'pio' found in PATH.${NC}"
    echo -e "You can provide a pre-compiled binary using: ./tools/compile_and_upload.sh --bin <path/to/firmware.bin>"
    exit 1
  fi
fi

if [[ ! -f "${BIN_FILE}" ]]; then
  echo -e "${RED}[ERROR] Compiled binary not found at: ${BIN_FILE}${NC}"
  exit 1
fi

BIN_SIZE=$(wc -c < "${BIN_FILE}" | tr -d ' ')
BIN_SIZE_KB=$(( BIN_SIZE / 1024 ))
echo -e "${MINT}[OK] Firmware binary ready: ${BIN_FILE} (${BIN_SIZE_KB} KB)${NC}"

# Upload over Wi-Fi
echo -e "\n${AMBER}[2/2] Uploading Firmware to http://${TARGET_IP}/update over Wi-Fi...${NC}"

HTTP_CODE=$(curl -s -o /tmp/orbit_ota_response.json -w "%{http_code}" \
  -X POST \
  -F "file=@${BIN_FILE}" \
  "http://${TARGET_IP}/update?target=flash")

if [[ "${HTTP_CODE}" == "200" ]]; then
  echo -e "${MINT}=================================================================${NC}"
  echo -e "${MINT} [SUCCESS] Firmware Flashed Successfully Over Wi-Fi!           ${NC}"
  echo -e "${MINT} Response: $(cat /tmp/orbit_ota_response.json 2>/dev/null)    ${NC}"
  echo -e "${MINT} ORBIT is now rebooting into the new firmware.                ${NC}"
  echo -e "${MINT}=================================================================${NC}"
else
  echo -e "${RED}[ERROR] Upload failed with HTTP status code: ${HTTP_CODE}${NC}"
  cat /tmp/orbit_ota_response.json 2>/dev/null || true
  echo -e "\n${AMBER}Tips: Ensure ORBIT is connected to the same Wi-Fi network and accessible at http://${TARGET_IP}.${NC}"
  exit 1
fi

