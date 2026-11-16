#!/bin/bash
# my_pipedal_install.sh - build and install PiPedal from the maarthome/pipedal fork
# (branch my_mods) on Raspberry Pi OS, optionally with the DSI touchscreen UI + kiosk.
#
# Run as the normal user (not root), from anywhere:
#   ./my_pipedal_install.sh               build + install; asks about the DSI screen at the end
#   ./my_pipedal_install.sh --dsi         ... and install the DSI touchscreen UI + kiosk, no question
#   ./my_pipedal_install.sh --no-dsi      ... without the DSI screen, no question
#   ./my_pipedal_install.sh --continue    continue an interrupted build (keeps the build folder)
#   PIPEDAL_DIR=/other/path ./my_pipedal_install.sh    use another source folder
# Default source folder: ~/git/pipedal
set -euo pipefail

REPO_URL="https://github.com/maarthome/pipedal"
BRANCH="my_mods"
DIR="${PIPEDAL_DIR:-$HOME/git/pipedal}"
DSI=ask; CONT=0
for a in "$@"; do
  case "$a" in
    --dsi) DSI=yes ;;
    --no-dsi) DSI=no ;;
    --continue) CONT=1 ;;
    *) sed -n '2,13p' "$0"; exit 1 ;;
  esac
done
[ "$(id -u)" -ne 0 ] || { echo "Run as the normal user, not as root."; exit 1; }

echo "== 1. Build tools and libraries"
sudo apt update
sudo apt install -y cmake ninja-build build-essential g++ git nodejs npm curl \
    liblilv-dev libboost-dev libsystemd-dev libasound2-dev uuid-dev \
    authbind libavahi-client-dev libnm-dev libicu-dev libsdbus-c++-dev \
    libzip-dev google-perftools libgoogle-perftools-dev libpipewire-0.3-dev \
    libbz2-dev libssl-dev librsvg2-dev
sudo apt install -y catch2 || sudo apt install -y catch
NODE_MAJOR=$(node --version 2>/dev/null | sed 's/^v//; s/\..*//')
if [ -z "$NODE_MAJOR" ] || [ "$NODE_MAJOR" -lt 22 ]; then
  echo "ERROR: PiPedal needs node.js 22 or later (found: $(node --version 2>/dev/null || echo none))."
  echo "Install a current node.js from https://nodejs.org/en/download and run this script again."
  exit 1
fi
MEM_MB=$(awk '/MemTotal/ {print int($2/1024)}' /proc/meminfo)
[ "$MEM_MB" -ge 1800 ] || echo "Warning: only ${MEM_MB} MB RAM; the build may run out of memory (add swap)."

echo "== 2. Source: $DIR ($BRANCH)"
if [ -d "$DIR/.git" ]; then
  git -C "$DIR" fetch origin "$BRANCH"
  git -C "$DIR" checkout "$BRANCH"
  git -C "$DIR" pull --ff-only origin "$BRANCH"
else
  mkdir -p "$(dirname "$DIR")"
  git clone --recurse-submodules --branch "$BRANCH" "$REPO_URL" "$DIR"
fi
cd "$DIR"
git submodule update --init --recursive

echo "== 3. React dependencies"
./react-config

echo "== 4. Build (takes a long time; --continue resumes an interrupted build)"
if [ $CONT -eq 1 ] && [ -d build ]; then ./build-prod.sh --continue; else ./build-prod.sh; fi

echo "== 5. Install the package"
DEB=$(ls -t build/pipedal_*.deb 2>/dev/null | head -1 || true)
[ -n "$DEB" ] || { echo "ERROR: no .deb found in $DIR/build"; exit 1; }
sudo apt-get install -y "./$DEB"
for i in $(seq 1 60); do curl -sf -o /dev/null http://localhost/ && break; sleep 1; done
if curl -sf -o /dev/null http://localhost/; then echo "PiPedal web interface answers on http://localhost"
else echo "Warning: http://localhost does not answer yet (check: systemctl status pipedald)"; fi

echo "== 6. DSI touchscreen UI + kiosk"
if [ "$DSI" = ask ]; then
  read -p "Install the DSI touchscreen UI and Chromium kiosk now? [y/N] " a
  [ "$a" = "y" ] && DSI=yes || DSI=no
fi
if [ "$DSI" = yes ]; then
  cp "$DIR"/dsi-ui/{pipedal-dsi-ui.patch,pipedal-dsi-ui.commit,dsi-ui-install.sh,kiosk-setup.sh,pipedal-run.sh,pipedal-remove.sh} "$HOME/"
  chmod +x "$HOME"/{dsi-ui-install.sh,kiosk-setup.sh,pipedal-run.sh,pipedal-remove.sh}
  "$HOME/kiosk-setup.sh"
  "$HOME/dsi-ui-install.sh"     # asks about the reboot at the end
else
  echo "Skipped. Later: cd $DIR/dsi-ui and see README.txt."
fi

echo "== Done. Helper scripts (if the DSI part was installed): ~/pipedal-run.sh, ~/pipedal-remove.sh"
