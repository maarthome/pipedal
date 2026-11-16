#!/bin/bash
# pipedal-remove.sh - remove PiPedal (and optionally the kiosk setup and all data).
# Run as the normal user, not as root. sudo is used where needed.
#
#   pipedal-remove.sh                 stop + disable PiPedal services, remove the package
#                                     (presets and settings are kept)
#   pipedal-remove.sh --kiosk         also remove the kiosk: pipedal-kiosk.service, any old
#                                     autologin, ~/.xserverrc and ~/.bash_profile restored from
#                                     their .bak files, old build folders
#   pipedal-remove.sh --purge-data    also delete /etc/pipedal and /var/pipedal
#                                     (ALL presets, pedalboards and settings)
#   add -y to skip the confirmation question
set -u
KIOSK=0; PURGE=0; YES=0
for a in "$@"; do
  case "$a" in
    --kiosk) KIOSK=1 ;;
    --purge-data) PURGE=1 ;;
    -y) YES=1 ;;
    *) sed -n '2,13p' "$0"; exit 1 ;;
  esac
done
[ "$(id -u)" -ne 0 ] || { echo "Run as the normal user, not as root."; exit 1; }

units() { systemctl list-unit-files --plain --no-legend 'pipedal*.service' 2>/dev/null | awk '{print $1}'; }

echo "This will:"
echo "  - stop and disable the PiPedal services: $(units | tr '\n' ' ')"
echo "  - remove the pipedal package (apt)"
[ $KIOSK -eq 1 ] && echo "  - remove the kiosk service, undo any old autologin setup (.xserverrc, .bash_profile) and delete ~/pipedal-build ~/pipedal-stock ~/react.stock"
[ $PURGE -eq 1 ] && echo "  - DELETE /etc/pipedal and /var/pipedal (all presets and settings!)"
if [ $YES -eq 0 ]; then
  read -p "Continue? [y/N] " a; [ "$a" = "y" ] || { echo "Cancelled."; exit 0; }
  if [ $PURGE -eq 1 ]; then
    read -p "Really delete ALL PiPedal presets and settings? Type 'delete': " a
    [ "$a" = "delete" ] || { echo "Cancelled."; exit 0; }
  fi
fi

echo "== Stopping and disabling services"
[ $KIOSK -eq 1 ] && sudo systemctl stop pipedal-kiosk.service 2>/dev/null
for u in $(units); do
  sudo systemctl stop "$u" 2>/dev/null
  sudo systemctl disable "$u" 2>/dev/null
done

echo "== Removing the package"
if dpkg -s pipedal >/dev/null 2>&1; then
  if [ $PURGE -eq 1 ]; then sudo apt purge -y pipedal; else sudo apt remove -y pipedal; fi
else
  echo "  package pipedal is not installed (nothing to remove)"
fi
sudo systemctl daemon-reload

if [ $PURGE -eq 1 ]; then
  echo "== Deleting data"
  sudo rm -rf /etc/pipedal /var/pipedal
fi

if [ $KIOSK -eq 1 ]; then
  echo "== Undoing the kiosk setup"
  sudo rm -f /etc/systemd/system/pipedal-kiosk.service
  sudo rm -f /etc/systemd/system/getty@tty1.service.d/autologin.conf /etc/systemd/system/getty@tty1.service.d/autologin.conf.off
  rm -f "$HOME/.pipedal-autostart"
  sudo rmdir /etc/systemd/system/getty@tty1.service.d 2>/dev/null
  sudo systemctl daemon-reload
  if [ -f "$HOME/xserverrc.bak" ]; then mv -f "$HOME/xserverrc.bak" "$HOME/.xserverrc"; else rm -f "$HOME/.xserverrc"; fi
  if [ -f "$HOME/bash_profile.bak" ]; then mv -f "$HOME/bash_profile.bak" "$HOME/.bash_profile"
  else
    # no backup: drop only a startx kiosk block, if an old autostart setup left one
    sed -i '/exec startx \/usr\/bin\/chromium/,/^fi$/d; /^if \[ -z "\$SSH_CONNECTION" \]/d' "$HOME/.bash_profile" 2>/dev/null
  fi
  sudo rm -rf "$HOME/pipedal-build" "$HOME/pipedal-stock" "$HOME/react.stock"
fi

echo "== Done. Packages such as chromium and xinit were not removed."
echo "Reboot to finish: sudo reboot"
