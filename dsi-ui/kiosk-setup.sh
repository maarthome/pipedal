#!/bin/bash
# Sets up the Chromium kiosk on a Raspberry Pi OS (Lite or desktop) console.
# Run as the normal user (not root). Uses sudo for the system parts.
set -e
U="$USER"

echo "== 1. Packages"
sudo apt update
sudo apt install -y xserver-xorg xinit x11-xserver-utils chromium

echo "== 2. Console autologin on tty1"
sudo mkdir -p /etc/systemd/system/getty@tty1.service.d
sudo tee /etc/systemd/system/getty@tty1.service.d/autologin.conf >/dev/null <<EOT
[Service]
ExecStart=
ExecStart=-/sbin/agetty --autologin $U --noclear %I \$TERM
EOT
sudo systemctl daemon-reload

echo "== 3. X server options (no screen blanking, no mouse pointer)"
[ -f "$HOME/.xserverrc" ] && cp "$HOME/.xserverrc" "$HOME/xserverrc.bak"
cat > "$HOME/.xserverrc" <<'EOT'
#!/bin/sh
exec /usr/bin/X -s 0 -dpms -nocursor "$@"
EOT
chmod +x "$HOME/.xserverrc"

echo "== 4. Start X and Chromium on login at tty1 (not over SSH)"
[ -f "$HOME/.bash_profile" ] && cp "$HOME/.bash_profile" "$HOME/bash_profile.bak"
cat > "$HOME/.bash_profile" <<'EOT'
# a login shell does not read ~/.bashrc by itself when ~/.bash_profile exists
[ -f "$HOME/.bashrc" ] && . "$HOME/.bashrc"
if [ -z "$SSH_CONNECTION" ] && [ "$(tty)" = "/dev/tty1" ]; then
  exec startx /usr/bin/chromium --kiosk --no-sandbox --noerrdialogs --disable-infobars --incognito http://localhost
fi
EOT

echo "== Done. PiPedal must already be installed and running."
echo "Reboot to start the kiosk."
