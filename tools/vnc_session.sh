#!/usr/bin/env bash
# Runs TYLI on a virtual X display (Xvfb) and shares that display over VNC,
# listening on 127.0.0.1 ONLY. Nothing is reachable from the network: you reach it
# through your SSH connection.
#
#   On the VM:          tools/vnc_session.sh               (Ctrl+C stops everything)
#   On your machine:    ssh -N -L 5900:127.0.0.1:5900 <user>@<vm-address>
#                       then point any VNC viewer at localhost:5900
#
# Options (environment variables): VNC_PORT (5900), DISPLAY_NUM (99), SCREEN (1440x900x24),
# TYLI_BIN (build/tyli). Extra arguments are passed to TYLI.
# Do NOT use Qt's own "-platform vnc": it listens on every network interface.

set -euo pipefail
cd "$(dirname "$0")/.."

VNC_PORT="${VNC_PORT:-5900}"
DISPLAY_NUM="${DISPLAY_NUM:-99}"
SCREEN="${SCREEN:-1440x900x24}"
TYLI_BIN="${TYLI_BIN:-build/tyli}"

[[ -x "$TYLI_BIN" ]] || { echo "build TYLI first: $TYLI_BIN not found" >&2; exit 1; }

Xvfb ":$DISPLAY_NUM" -screen 0 "$SCREEN" -nolisten tcp &
XVFB_PID=$!
VNC_PID=""
trap 'kill ${VNC_PID:-} "$XVFB_PID" 2>/dev/null || true' EXIT
sleep 1

# -localhost: bind 127.0.0.1 only. -nopw is acceptable only because of that binding.
x11vnc -display ":$DISPLAY_NUM" -localhost -rfbport "$VNC_PORT" -forever -shared -nopw -quiet -bg -o /dev/null >/dev/null 2>&1 &
VNC_PID=$!
sleep 1

echo "TYLI is on a virtual display, shared over VNC at 127.0.0.1:${VNC_PORT} (this VM only)."
echo "From your machine:  ssh -N -L ${VNC_PORT}:127.0.0.1:${VNC_PORT} <user>@<vm-address>   then a VNC viewer -> localhost:${VNC_PORT}"

# Software rendering: there's no GPU on the VM, and it's fine for a UI of lists and forms.
DISPLAY=":$DISPLAY_NUM" QT_QPA_PLATFORM=xcb QT_QUICK_BACKEND=software "$TYLI_BIN" "$@"
