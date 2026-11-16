#!/bin/bash
# my_pipedal_install.sh - build and install PiPedal from the maarthome/pipedal fork
# (branch my_mods) on Raspberry Pi OS, optionally with a manually started touchscreen kiosk.
# The 7-inch DSI interface changes are part of the my_mods source, so the build already has them.
#
# Run as the normal user (not root), from anywhere:
#   ./my_pipedal_install.sh               build + install; asks about manual kiosk start/stop at the end
#   ./my_pipedal_install.sh --dsi         ... and set up manual kiosk start/stop, no question
#   ./my_pipedal_install.sh --no-dsi      ... without the kiosk, no question
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
# The KXStudio repository ships an lv2-dev without /usr/include/lv2.h that breaks the build.
# Prefer Debian's lv2-dev (no effect on systems without that repository).
printf 'Package: lv2-dev\nPin: release o=Debian\nPin-Priority: 1001\n' | sudo tee /etc/apt/preferences.d/lv2-dev-debian >/dev/null
sudo apt update
sudo apt install -y cmake ninja-build build-essential g++ git nodejs npm curl \
    lv2-dev liblilv-dev libboost-dev libsystemd-dev libasound2-dev uuid-dev \
    authbind libavahi-client-dev libnm-dev libicu-dev libsdbus-c++-dev \
    libzip-dev google-perftools libgoogle-perftools-dev libpipewire-0.3-dev \
    libbz2-dev libssl-dev librsvg2-dev
sudo apt install -y catch2 || sudo apt install -y catch
# Verify the LV2 headers; repair a broken KXStudio lv2-dev by installing Debian's version.
if [ ! -f /usr/include/lv2.h ] || [ -n "$(dpkg -V lv2-dev 2>/dev/null)" ]; then
  echo "LV2 headers are missing or incomplete (KXStudio lv2-dev?). Installing Debian's lv2-dev..."
  # stray symlinks (not owned by any package) block the Debian package
  for l in /usr/include/lv2/lv2plug.in/ns/ext /usr/include/lv2/lv2plug.in/ns/extensions; do
    [ -L "$l" ] && sudo rm -f "$l"
  done
  LV2_VER=$(apt-cache madison lv2-dev | awk -F'|' '/deb.debian.org/ {gsub(/ /,"",$2); print $2; exit}')
  [ -n "$LV2_VER" ] || { echo "ERROR: no Debian lv2-dev version found (apt-cache madison lv2-dev)."; exit 1; }
  sudo apt install -y --allow-downgrades "lv2-dev=$LV2_VER"
  if [ ! -f /usr/include/lv2.h ] || [ -n "$(dpkg -V lv2-dev 2>/dev/null)" ]; then
    echo "ERROR: LV2 headers are still broken. Check: dpkg -V lv2-dev ; ls -l /usr/include/lv2.h"
    exit 1
  fi
fi
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

echo "== 6. Touchscreen kiosk: start and stop by hand, nothing starts at boot"
chmod +x "$DIR"/pipedal-run.sh "$DIR"/pipedal-remove.sh "$DIR"/pipedal-start.sh "$DIR"/pipedal-stop.sh   # the scripts are used from the repository folder
if [ "$DSI" = ask ]; then
  read -p "Set up manual start/stop of PiPedal + the touchscreen kiosk (no autostart at boot)? [y/N] " a
  [ "$a" = "y" ] && DSI=yes || DSI=no
fi
if [ "$DSI" = yes ]; then
  "$DIR/pipedal-run.sh" manual-mode
else
  echo "Skipped. Later: $DIR/pipedal-run.sh manual-mode"
fi

echo "== Done. Start everything:  $DIR/pipedal-start.sh     Stop everything:  $DIR/pipedal-stop.sh"
