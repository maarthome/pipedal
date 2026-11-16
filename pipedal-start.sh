#!/bin/bash
# pipedal-start.sh - start PiPedal (services) and the touchscreen kiosk (X + Chromium).
# Nothing starts at boot; this is the way to start it. Run as the normal user.
exec "$(dirname "$(readlink -f "$0")")/pipedal-run.sh" start "$@"
