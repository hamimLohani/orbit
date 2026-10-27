#!/bin/bash
# ==============================================================================
# ORBIT v2 — Wi-Fi File Upload Tool (SD Card / LittleFS)
# Usage:
#   ./tools/upload_file.sh <local_file> [target_path] [sd|lfs] [device_ip]
# Examples:
#   ./tools/upload_file.sh my_wallpaper.bmp /images/wallpapers sd orbit.local
#   ./tools/upload_file.sh app.js /web lfs orbit.local
# ==============================================================================

set -e

CYAN='\033[0;36m'
MINT='\033[0;32m'
AMBER='\033[0;33m'
RED='\033[0;31m'
NC='\033[0m'

LOCAL_FILE="$1"
TARGET_PATH="${2:-/}"
STORAGE_TARGET="${3:-sd}"
DEVICE_IP="${4:-orbit.local}"

if [[ -z "${LOCAL_FILE}" || ! -f "${LOCAL_FILE}" ]]; then
  echo -e "${RED}[ERROR] Local file not found: '${LOCAL_FILE}'${NC}"
  echo -e "Usage: $0 <local_file> [target_path] [sd|lfs] [device_ip]"
  exit 1
fi

FILE_SIZE=$(wc -c < "${LOCAL_FILE}" | tr -d ' ')
FILE_SIZE_KB=$(( FILE_SIZE / 1024 ))
FILENAME=$(basename "${LOCAL_FILE}")

echo -e "${CYAN}=================================================================${NC}"
echo -e "${CYAN}   ORBIT v2 — Wi-Fi File Uploader                               ${NC}"
echo -e "${CYAN}=================================================================${NC}"
echo -e "File:     ${FILENAME} (${FILE_SIZE_KB} KB)"
echo -e "Target:   ${STORAGE_TARGET} -> ${TARGET_PATH}"
echo -e "Device:   http://${DEVICE_IP}"

HTTP_CODE=$(curl -s -o /tmp/orbit_file_response.json -w "%{http_code}" \
  -X POST \
  -F "file=@${LOCAL_FILE}" \
  "http://${DEVICE_IP}/api/files/upload?target=${STORAGE_TARGET}&path=${TARGET_PATH}")

if [[ "${HTTP_CODE}" == "200" ]]; then
  echo -e "${MINT}[SUCCESS] File '${FILENAME}' uploaded to ${STORAGE_TARGET}:${TARGET_PATH} over Wi-Fi!${NC}"
  cat /tmp/orbit_file_response.json 2>/dev/null || true
  echo ""
else
  echo -e "${RED}[ERROR] File upload failed with HTTP status: ${HTTP_CODE}${NC}"
  cat /tmp/orbit_file_response.json 2>/dev/null || true
  exit 1
fi

