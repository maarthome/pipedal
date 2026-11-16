#!/bin/bash
# pipedal-run.sh - start, stop, restart and check PiPedal and its touchscreen kiosk.
# Run as the normal user (maart). sudo is used for the system parts.
#
#   pipedal-run.sh status           show services, web server, installed UI, kiosk
#   pipedal-run.sh start            start PiPedal services, then the kiosk screen
#   pipedal-run.sh stop             stop the kiosk screen and the PiPedal services
#   pipedal-run.sh restart          restart PiPedal services and the kiosk screen
#   pipedal-run.sh restart-kiosk    only restart the kiosk screen (e.g. after a UI install)
#   pipedal-run.sh stop-kiosk       only stop the kiosk screen
#   pipedal-run.sh restart-pipedal  only restart the PiPedal services (audio server)
set -u

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
    if systemctl is-enabled --quiet "$u" 2>/dev/null; then
      sudo systemctl start "$u"
    fi
  done
  wait_web && echo "PiPedal web server is up."
}

stop_services() {
  for u in $(units); do
    sudo systemctl stop "$u" 2>/dev/null
  done
}

# The kiosk is started by the autologin on tty1 (see ~/.bash_profile),
# so restarting getty@tty1 restarts the whole screen session.
kiosk_running() { pgrep -x chromium >/dev/null; }
start_kiosk() {
  if kiosk_running; then echo "Kiosk already running."; else sudo systemctl restart getty@tty1; echo "Kiosk starting on the touchscreen."; fi
}
stop_kiosk() { sudo systemctl stop getty@tty1; echo "Kiosk stopped."; }
restart_kiosk() { sudo systemctl restart getty@tty1; echo "Kiosk restarting on the touchscreen."; }

status() {
  echo "== PiPedal services"
  for u in $(units); do printf '  %-32s %s\n' "$u" "$(systemctl is-active "$u")"; done
  echo "== Web server"
  echo "  http://localhost -> HTTP $(curl -s -o /dev/null -w '%{http_code}' http://localhost/ 2>/dev/null)"
  echo "== Installed web interface"
  ls /etc/pipedal/react/assets 2>/dev/null | grep -E '^main-.*\.js$' | sed 's/^/  /'
  if grep -qs pipedal-toggle-fit /etc/pipedal/react/assets/*.js; then echo "  -> DSI-customised"; else echo "  -> original"; fi
  echo "== Kiosk"
  if kiosk_running; then echo "  Chromium is running"; else echo "  Chromium is not running"; fi
}

case "${1:-}" in
  status)          status ;;
  start)           start_services; start_kiosk ;;
  stop)            stop_kiosk; stop_services ;;
  restart)         stop_kiosk; sleep 1; stop_services; start_services; start_kiosk ;;
  restart-kiosk)   restart_kiosk ;;
  stop-kiosk)      stop_kiosk ;;
  restart-pipedal) stop_services; start_services ;;
  *) sed -n '2,12p' "$0"; exit 1 ;;
esac
