#!/usr/bin/env bash
# Serves the HTML mockups (DOCUMENTATION/mockups/) on 127.0.0.1 ONLY, so you can watch
# them change live from your own machine through SSH. Each page reloads itself when it
# (or theme.css) changes on disk.
#
#   On the VM:          tools/mockup_server.sh                 (Ctrl+C stops it)
#   On your machine:    ssh -N -L 8000:127.0.0.1:8000 <user>@<vm-address>
#                       then open http://localhost:8000
#
# Option (environment variable): MOCKUP_PORT (8000).

set -euo pipefail
cd "$(dirname "$0")/../DOCUMENTATION/mockups"
PORT="${MOCKUP_PORT:-8000}"
echo "Mockups at http://127.0.0.1:${PORT} (this VM only)."
exec python3 -m http.server "$PORT" --bind 127.0.0.1
