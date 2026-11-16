#!/bin/bash
# pipedal-run.sh - start, stop, restart and check PiPedal and its touchscreen kiosk.
# Run as the normal user (maart). sudo is used for the system parts.
#
#   pipedal-run.sh status             services, web server, installed UI, kiosk, autostart
#   pipedal-run.sh start              start PiPedal services (+ kiosk if screen autologin is on)
#   pipedal-run.sh stop               stop the kiosk screen and the PiPedal services
#   pipedal-run.sh restart            restart PiPedal services and the kiosk screen
#   pipedal-run.sh kiosk              start the kiosk screen by hand (run on the Pi's own console, not SSH)
#   pipedal-run.sh restart-kiosk      only restart the kiosk screen (autologin setup)
#   pipedal-run.sh stop-kiosk         only stop the kiosk screen
#   pipedal-run.sh restart-pipedal    only restart the PiPedal services (audio server)
#   pipedal-run.sh autostart off|on   do not / do start the PiPedal services at boot
#   pipedal-run.sh kiosk-autostart off|on   do not / do log in and start the kiosk at boot
set -u

AUTOLOGIN=/etc/systemd/system/getty@tty1.service.d/autologin.conf
STATE="$HOME/.pipedal-autostart"
CORE="pipedald.service pipedaladmind.service"

units() { systemctl list-unit-files --plain --no-legend 'pipedal*.service' 2>/dev/null | awk '{print $1}'; }

wait_web() {
  for i in $(seq 1 40); do
    curl -sf -o /dev/null http://localhost/ && return 0
    sleep 1
  done
  echo "Warning: http://localhost does not answer yet."
  return 1
}

start_services() {
  for u in $(units); do
    # the core services are started even when they are not enabled at boot
    if echo " $CORE " | grep -q " $u " || systemctl is-enabled --quiet "$u" 2>/dev/null; then
      sudo systemctl start "$u"
    fi
  done
  wait_web && echo "PiPedal web server is up."
}

stop_services() {
  for u in $(units); do sudo systemctl stop "$u" 2>/dev/null; done
}

autologin_on() { [ -f "$AUTOLOGIN" ]; }
kiosk_running() { pgrep -x chromium >/dev/null; }

start_kiosk() {
  if kiosk_running; then echo "Kiosk already running."
  elif autologin_on; then sudo systemctl restart getty@tty1; echo "Kiosk starting on the touchscreen."
  else echo "Screen autologin is off. Start the kiosk on the Pi's own console with:  ~/pipedal-run.sh kiosk"; fi
}
stop_kiosk() {
  autologin_on && sudo systemctl stop getty@tty1
  pkill -x chromium 2>/dev/null
  echo "Kiosk stopped."
}
restart_kiosk() {
  if autologin_on; then sudo systemctl restart getty@tty1; echo "Kiosk restarting on the touchscreen."
  else echo "Screen autologin is off; use:  ~/pipedal-run.sh kiosk  (on the console)"; fi
}

run_kiosk() {
  if [ -n "${SSH_CONNECTION:-}" ]; then echo "Run this on the Pi's own console (tty1), not over SSH."; exit 1; fi
  exec startx /usr/bin/chromium --kiosk --no-sandbox --noerrdialogs --disable-infobars --incognito http://localhost -- -s 0 -dpms -nocursor
}

autostart() {
  case "${1:-}" in
    off)
      : > "$STATE"
      for u in $(units); do
        if systemctl is-enabled --quiet "$u" 2>/dev/null; then echo "$u" >> "$STATE"; sudo systemctl disable "$u"; fi
      done
      echo "PiPedal services will not start at boot. Start them with:  ~/pipedal-run.sh start" ;;
    on)
      if [ -s "$STATE" ]; then for u in $(cat "$STATE"); do sudo systemctl enable "$u"; done
      else for u in $CORE; do sudo systemctl enable "$u"; done; fi
      echo "PiPedal services will start at boot." ;;
    *) echo "usage: pipedal-run.sh autostart off|on"; exit 1 ;;
  esac
}

kiosk_autostart() {
  case "${1:-}" in
    off)
      if [ -f "$AUTOLOGIN" ]; then
        sudo mv "$AUTOLOGIN" "$AUTOLOGIN.off"; sudo systemctl daemon-reload
        echo "No automatic login at boot. Logging in on tty1 by hand still starts the kiosk (~/.bash_profile)."
      else echo "Screen autologin was already off."; fi ;;
    on)
      if [ -f "$AUTOLOGIN.off" ]; then
        sudo mv "$AUTOLOGIN.off" "$AUTOLOGIN"; sudo systemctl daemon-reload
        echo "The Pi logs in and starts the kiosk at boot."
      else echo "No saved autologin setup found (run ~/kiosk-setup.sh)."; fi ;;
    *) echo "usage: pipedal-run.sh kiosk-autostart off|on"; exit 1 ;;
  esac
}

status() {
  echo "== PiPedal services"
  for u in $(units); do printf '  %-32s %-10s boot: %s\n' "$u" "$(systemctl is-active "$u")" "$(systemctl is-enabled "$u" 2>/dev/null)"; done
  [ -z "$(units)" ] && echo "  no pipedal services found"
  echo "== Web server"
  echo "  http://localhost -> HTTP $(curl -s -o /dev/null -w '%{http_code}' http://localhost/ 2>/dev/null)"
  echo "== Installed web interface"
  ls /etc/pipedal/react/assets 2>/dev/null | grep -E '^main-.*\.js$' | sed 's/^/  /'
  if [ ! -d /etc/pipedal/react/assets ]; then echo "  PiPedal web interface not installed (/etc/pipedal/react missing)"
  elif grep -qs pipedal-toggle-fit /etc/pipedal/react/assets/*.js; then echo "  -> DSI-customised"
  else echo "  -> original"; fi
  echo "== Kiosk"
  if kiosk_running; then echo "  Chromium is running"; else echo "  Chromium is not running"; fi
  if autologin_on; then echo "  screen autologin at boot: on"; else echo "  screen autologin at boot: off"; fi
}

case "${1:-}" in
  status)          status ;;
  start)           start_services; start_kiosk ;;
  stop)            stop_kiosk; stop_services ;;
  restart)         stop_kiosk; sleep 1; stop_services; start_services; start_kiosk ;;
  kiosk)           run_kiosk ;;
  restart-kiosk)   restart_kiosk ;;
  stop-kiosk)      stop_kiosk ;;
  restart-pipedal) stop_services; start_services ;;
  autostart)       autostart "${2:-}" ;;
  kiosk-autostart) kiosk_autostart "${2:-}" ;;
  *) sed -n '2,15p' "$0"; exit 1 ;;
esac
