#!/bin/bash
# pipedal-run.sh - start and stop PiPedal AND the touchscreen kiosk (Chromium) by hand.
# Nothing starts at boot. Run as the normal user (not root); sudo is used where needed.
#
#   pipedal-run.sh start              start the PiPedal services, then the kiosk screen (X + Chromium)
#   pipedal-run.sh stop               stop the kiosk screen and the PiPedal services
#   pipedal-run.sh restart            stop, then start everything
#   pipedal-run.sh status             services, web server, installed UI, kiosk
#   pipedal-run.sh start-kiosk | stop-kiosk | restart-kiosk     only the screen
#   pipedal-run.sh restart-pipedal    only the PiPedal services (audio server)
#   pipedal-run.sh manual-mode        ONE-TIME setup: no autostart of PiPedal or the kiosk at boot
#   pipedal-run.sh update-kiosk       rewrite the kiosk service file (after changes in this script)
#   pipedal-run.sh autostart on|off   PiPedal services at boot (manual-mode switches this off)
set -u

UNIT=/etc/systemd/system/pipedal-kiosk.service
AUTOLOGIN=/etc/systemd/system/getty@tty1.service.d/autologin.conf
STATE="$HOME/.pipedal-autostart"
CORE="pipedald.service pipedaladmind.service"
CHROMIUM_FLAGS="${CHROMIUM_FLAGS:-}"   # extra Chromium flags, written into the kiosk service by update-kiosk
KIOSK_DELAY="${KIOSK_DELAY:-0}"   # seconds between "web server answers" and starting Chromium

# the PiPedal services, WITHOUT the kiosk service (that one is started last, by start_kiosk)
units() { systemctl list-unit-files --plain --no-legend 'pipedal*.service' 2>/dev/null | awk '{print $1}' | grep -v '^pipedal-kiosk\.service$'; }

wait_web() {
  for i in $(seq 1 40); do curl -sf -o /dev/null http://localhost/ && return 0; sleep 1; done
  echo "Warning: http://localhost does not answer yet."; return 1
}

start_services() {
  for u in $(units); do
    # the core services are started even though they are not enabled at boot
    if echo " $CORE " | grep -q " $u " || systemctl is-enabled --quiet "$u" 2>/dev/null; then sudo systemctl start "$u"; fi
  done
  if wait_web; then
    echo "PiPedal web server is up."
    [ "$KIOSK_DELAY" -gt 0 ] && sleep "$KIOSK_DELAY"
  fi
}
stop_services() { for u in $(units); do sudo systemctl stop "$u" 2>/dev/null; done; }

# The kiosk is a systemd service that is never enabled: it only runs after "start".
install_kiosk_unit() {
  sudo tee "$UNIT" >/dev/null <<EOT
[Unit]
Description=PiPedal touchscreen kiosk (X + Chromium)
After=pipedald.service
Conflicts=getty@tty1.service

[Service]
User=$USER
PAMName=login
TTYPath=/dev/tty1
TTYReset=yes
TTYVHangup=yes
TTYVTDisallocate=yes
StandardInput=tty-force
StandardOutput=journal
StandardError=journal
UtmpIdentifier=tty1
UtmpMode=user
ExecStart=/usr/bin/startx /usr/bin/chromium --kiosk --no-sandbox --noerrdialogs --disable-infobars --incognito $CHROMIUM_FLAGS http://localhost -- -nocursor
ExecStop=-/usr/bin/pkill -TERM -x chromium
TimeoutStopSec=8
KillMode=control-group
ExecStopPost=-+/bin/systemctl --no-block start getty@tty1.service
EOT
  sudo systemctl daemon-reload
}
kiosk_running() { systemctl is-active --quiet pipedal-kiosk.service; }
start_kiosk() {
  [ -f "$UNIT" ] || { echo "Kiosk service not set up yet: run  $0 manual-mode"; return 1; }
  if kiosk_running; then echo "Kiosk already running."; else sudo systemctl start pipedal-kiosk.service && echo "Kiosk starting on the touchscreen."; fi
}
stop_kiosk() { [ -f "$UNIT" ] && sudo systemctl stop pipedal-kiosk.service; echo "Kiosk stopped."; }

autostart() {
  case "${1:-}" in
    off) : > "$STATE"
         for u in $(units); do
           if systemctl is-enabled --quiet "$u" 2>/dev/null; then echo "$u" >> "$STATE"; sudo systemctl disable "$u"; fi
         done
         echo "PiPedal services will not start at boot." ;;
    on)  if [ -s "$STATE" ]; then for u in $(cat "$STATE"); do sudo systemctl enable "$u"; done
         else for u in $CORE; do sudo systemctl enable "$u"; done; fi
         echo "PiPedal services will start at boot." ;;
    *)   echo "usage: $0 autostart on|off"; exit 1 ;;
  esac
}

manual_mode() {
  echo "== Packages for the kiosk"
  sudo apt install -y xserver-xorg xinit x11-xserver-utils chromium
  echo "== X server options (no screen blanking, no mouse pointer)"
  if [ ! -f "$HOME/.xserverrc" ]; then
    printf '#!/bin/sh\nexec /usr/bin/X -s 0 -dpms -nocursor "$@"\n' > "$HOME/.xserverrc"; chmod +x "$HOME/.xserverrc"
  fi
  echo "== No automatic login / kiosk start at boot"
  if [ -f "$AUTOLOGIN" ]; then sudo mv "$AUTOLOGIN" "$AUTOLOGIN.off"; sudo systemctl daemon-reload; echo "  autologin switched off (saved as autologin.conf.off)"; fi
  if grep -qs 'exec startx' "$HOME/.bash_profile"; then
    cp "$HOME/.bash_profile" "$HOME/bash_profile.manual.bak"
    sed -i '/exec startx/,/^fi$/d; /SSH_CONNECTION/d' "$HOME/.bash_profile"
    echo "  removed the startx block from ~/.bash_profile (backup: ~/bash_profile.manual.bak)"
  fi
  autostart off
  echo "== Kiosk service (not enabled at boot)"
  install_kiosk_unit
  sudo systemctl disable pipedal-kiosk.service 2>/dev/null
  echo "Done. Start everything with:  $0 start      Stop everything with:  $0 stop"
}

status() {
  echo "== PiPedal services"
  for u in $(units); do printf '  %-32s %-10s at boot: %s\n' "$u" "$(systemctl is-active "$u")" "$(systemctl is-enabled "$u" 2>/dev/null)"; done
  [ -z "$(units)" ] && echo "  no pipedal services found"
  echo "== Web server"
  echo "  http://localhost -> HTTP $(curl -s -o /dev/null -w '%{http_code}' http://localhost/ 2>/dev/null)"
  echo "== Installed web interface"
  ls /etc/pipedal/react/assets 2>/dev/null | grep -E '^main-.*\.js$' | sed 's/^/  /'
  if [ ! -d /etc/pipedal/react/assets ]; then echo "  PiPedal web interface not installed (/etc/pipedal/react missing)"
  elif grep -qs pipedal-toggle-fit /etc/pipedal/react/assets/*.js; then echo "  -> DSI-customised"; else echo "  -> original"; fi
  echo "== Kiosk"
  if [ ! -f "$UNIT" ]; then echo "  not set up (run: $0 manual-mode)"
  else echo "  pipedal-kiosk.service: $(systemctl is-active pipedal-kiosk.service), at boot: $(systemctl is-enabled pipedal-kiosk.service 2>/dev/null)"; fi
  [ -f "$AUTOLOGIN" ] && echo "  WARNING: console autologin is still on (run: $0 manual-mode)"
}

[ "$(id -u)" -ne 0 ] || { echo "Run as the normal user, not as root."; exit 1; }
case "${1:-}" in
  start)           start_services; start_kiosk ;;
  stop)            stop_kiosk; stop_services ;;
  restart)         stop_kiosk; stop_services; sleep 1; start_services; start_kiosk ;;
  status)          status ;;
  start-kiosk)     start_kiosk ;;
  stop-kiosk)      stop_kiosk ;;
  restart-kiosk)   stop_kiosk; sleep 1; start_kiosk ;;
  restart-pipedal) stop_services; start_services ;;
  manual-mode)     manual_mode ;;
  update-kiosk)    install_kiosk_unit; echo "Kiosk service file updated (extra Chromium flags: ${CHROMIUM_FLAGS:-none})." ;;
  autostart)       autostart "${2:-}" ;;
  *) sed -n '2,14p' "$0"; exit 1 ;;
esac
