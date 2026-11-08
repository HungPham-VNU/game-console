#!/usr/bin/env bash
# Run PlatformIO inside Docker. Usage:
#   ./pio.sh run                      # build
#   ./pio.sh run -t upload            # build + flash (/dev/ttyACM0)
#   ./pio.sh device monitor           # serial monitor (Ctrl+C to quit)
#   ./pio.sh bootlog [seconds]        # reset the board and print its boot log
# Toolchains are cached in ~/.cache/pio-docker, so only the first run is slow.
set -euo pipefail
cd "$(dirname "$0")"

IMAGE=brick-game-pio
PORT=${PORT:-/dev/ttyACM0}
CACHE="$HOME/.cache/pio-docker"
mkdir -p "$CACHE"

if ! docker image inspect "$IMAGE" >/dev/null 2>&1; then
  docker build -t "$IMAGE" docker
fi

args=(--rm -v "$PWD:/project" -v "$CACHE:/cache" -e HOME=/cache
      --user "$(id -u):$(id -g)")
# Serial device. The ESP32-S3's USB-Serial/JTAG port disappears and comes back on every
# reset, so pass all of /dev plus a cgroup rule for ttyACM (major 166) instead of
# --device (which would pin the old node). The container runs as our uid, so add the
# device's group to it; this also avoids needing the dialout group on the host.
if [ -e "$PORT" ]; then
  args+=(-v /dev:/dev --device-cgroup-rule='c 166:* rmw' --group-add "$(stat -c %g "$PORT")")
fi
[ -t 0 ] && args+=(-it)

if [ "${1:-}" = "bootlog" ]; then
  exec docker run "${args[@]}" --entrypoint python "$IMAGE" -u tools/bootlog.py "$PORT" "${2:-10}"
fi
exec docker run "${args[@]}" "$IMAGE" "$@"
