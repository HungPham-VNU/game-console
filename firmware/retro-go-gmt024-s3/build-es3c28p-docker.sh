#!/usr/bin/env bash
# Build (and optionally flash) Retro-Go for the ES3C28P / ES3N28P board in Docker.
# No host ESP-IDF needed. Usage:
#   ./build-es3c28p-docker.sh            # build retro-go_<ver>_es3c28p.img
#   ./build-es3c28p-docker.sh flash      # build, then write the .img to /dev/ttyACM0
#   APPS="launcher retro-core gwenesis" ./build-es3c28p-docker.sh   # choose apps
set -euo pipefail
cd "$(dirname "$0")"

IMAGE=espressif/idf:v4.4.8
TARGET=es3c28p
APPS=${APPS:-"launcher retro-core"}
PORT=${PORT:-/dev/ttyACM0}

# The two ESP-IDF 4 patches are applied inside the throwaway container each run, so the
# image stays pristine. Files are chowned back to us at the end (the container is root).
docker run --rm -v "$PWD:/project" -w /project \
  -e HOST_UID="$(id -u)" -e HOST_GID="$(id -g)" -e APPS="$APPS" -e TARGET="$TARGET" \
  "$IMAGE" bash -c '
    set -e
    cd /opt/esp/idf
    for p in "/project/tools/patches/panic-hook (esp-idf 4).diff" "/project/tools/patches/sdcard-fix (esp-idf 4).diff"; do
      patch --forward --ignore-whitespace -p1 -i "$p" >/dev/null || true
    done
    cd /project
    . /opt/esp/idf/export.sh >/dev/null
    git config --global --add safe.directory "*"
    status=0
    python rg_tool.py --target="$TARGET" build-img $APPS || status=$?
    chown -R "$HOST_UID:$HOST_GID" /project
    exit $status
  '

IMG=$(ls -t retro-go_*_"$TARGET".img 2>/dev/null | head -1)
echo "Image: $IMG"

if [ "${1:-}" = "flash" ]; then
  [ -e "$PORT" ] || { echo "No $PORT"; exit 1; }
  docker run --rm -v "$PWD:/project" -w /project -v /dev:/dev --device-cgroup-rule='c 166:* rmw' \
    python:3.12-slim sh -c "pip install -q esptool 2>/dev/null && esptool --chip esp32s3 --port $PORT -b 460800 write-flash 0 '$IMG'"
fi
