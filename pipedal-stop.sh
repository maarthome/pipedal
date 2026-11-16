#!/bin/bash
# pipedal-stop.sh - stop the touchscreen kiosk (Chromium + X) and the PiPedal services.
# Run as the normal user.
exec "$(dirname "$(readlink -f "$0")")/pipedal-run.sh" stop "$@"
