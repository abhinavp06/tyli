#!/usr/bin/env bash
# Runs TYLI on a virtual X display (Xvfb) and shares that display over VNC,
# listening on 127.0.0.1 ONLY. Nothing is reachable from the network: you reach it
# through your SSH connection.
#
#   On the VM:          tools/vnc_session.sh               (Ctrl+C stops everything)
#   On your machine:    ssh -N -L 5900:127.0.0.1:5900 <user>@<vm-address>
#                       then point any VNC viewer at localhost:5900
#
# --watch (first argument): keep the display and VNC up, and restart TYLI whenever its binary
# changes (after every rebuild) or it exits. The viewer stays connected; the window just reloads.
#
# Options (environment variables): VNC_PORT (5900), DISPLAY_NUM (99), SCREEN (1440x900x24),
# TYLI_BIN (build/tyli). Extra arguments are passed to TYLI.
# Do NOT use Qt's own "-platform vnc": it listens on every network interface.

set -euo pipefail
cd "$(dirname "$0")/.."

WATCH=0
[[ "${1:-}" == "--watch" ]] && { WATCH=1; shift; }

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

# -localhost: bind 127.0.0.1 only. A VNC password is used when ~/.vnc/tyli_passwd exists
# (create it with: x11vnc -storepasswd <password> ~/.vnc/tyli_passwd). macOS Screen Sharing
# insists on one. Only the first 8 characters of a VNC password count.
AUTH=(-nopw)
[[ -f "$HOME/.vnc/tyli_passwd" ]] && AUTH=(-rfbauth "$HOME/.vnc/tyli_passwd")
x11vnc -display ":$DISPLAY_NUM" -localhost -rfbport "$VNC_PORT" -forever -shared "${AUTH[@]}" -quiet -bg -o /dev/null >/dev/null 2>&1 &
VNC_PID=$!
sleep 1

echo "TYLI is on a virtual display, shared over VNC at 127.0.0.1:${VNC_PORT} (this VM only)."
echo "From your machine:  ssh -N -L ${VNC_PORT}:127.0.0.1:${VNC_PORT} <user>@<vm-address>   then a VNC viewer -> localhost:${VNC_PORT}"

# Software rendering: there's no GPU on the VM, and it's fine for a UI of lists and forms.
export DISPLAY=":$DISPLAY_NUM" QT_QPA_PLATFORM=xcb QT_QUICK_BACKEND=software
# There's no window manager to maximize the window, so ask Qt to open it at the screen's size.
set -- -qwindowgeometry "${SCREEN%x*}+0+0" "$@"

if (( ! WATCH )); then
    "$TYLI_BIN" "$@"
    exit
fi

echo "Watching $TYLI_BIN: TYLI restarts after every rebuild. Ctrl+C stops everything."
mtime() { stat -c %Y "$TYLI_BIN" 2>/dev/null || echo 0; }
TYLI_PID=""
trap 'kill ${TYLI_PID:-} ${VNC_PID:-} "$XVFB_PID" 2>/dev/null || true' EXIT
while true; do
    seen=$(mtime)
    # A plain command (not a function) so that $! is TYLI itself, not a subshell around it.
    "$TYLI_BIN" "$@" &
    TYLI_PID=$!
    # Restart when the binary changes (and has stopped changing, so the linker is done),
    # or when TYLI exits on its own (closed or crashed).
    while kill -0 "$TYLI_PID" 2>/dev/null; do
        sleep 1
        now=$(mtime)
        if [[ "$now" != "$seen" ]]; then
            sleep 1
            [[ "$(mtime)" == "$now" && -x "$TYLI_BIN" ]] && break
        fi
    done
    kill "$TYLI_PID" 2>/dev/null || true
    wait "$TYLI_PID" 2>/dev/null || true
    echo "$(date +%T) restarting TYLI"
    sleep 1
done
