#!/bin/bash
# Rebuild the PiPedal web UI with the DSI-screen changes and install it.
set -euo pipefail
PATCH="$HOME/pipedal-dsi-ui.patch"
COMMIT_FILE="$HOME/pipedal-dsi-ui.commit"
WORK="$HOME/pipedal-build"

[ -f "$PATCH" ] || { echo "Missing $PATCH"; exit 1; }
[ -f "$COMMIT_FILE" ] || { echo "Missing $COMMIT_FILE"; exit 1; }
COMMIT=$(cat "$COMMIT_FILE")

echo "== 1. Packages"
sudo apt install -y git nodejs npm

echo "== 2. Fresh source at the recorded version"
rm -rf "$WORK"
git clone https://github.com/rerdavies/pipedal "$WORK"
cd "$WORK"
git checkout "$COMMIT"
git apply --check "$PATCH"   # stops here if the patch does not fit
git apply "$PATCH"

echo "== 3. Build"
cd vite
npm install
NODE_OPTIONS=--max-old-space-size=2048 ./build.sh

echo "== 4. Install (original UI is kept in ~/react.stock)"
if [ ! -d "$HOME/react.stock" ]; then
  if grep -qs "pipedal-toggle-fit" /etc/pipedal/react/assets/*.js; then
    echo "ERROR: ~/react.stock is missing and the installed UI is already modified."
    echo "Restore an original copy first (see the document, section 8)."; exit 1
  fi
  sudo cp -a /etc/pipedal/react "$HOME/react.stock"
fi
sudo rm -rf /etc/pipedal/react/assets
sudo cp -r dist/* /etc/pipedal/react/

echo "== 5. Hide the mouse pointer on the kiosk"
if ! grep -q -- '-nocursor' "$HOME/.xserverrc"; then
  cp "$HOME/.xserverrc" "$HOME/xserverrc.bak"
  sed -i 's|-dpms "\$@"|-dpms -nocursor "$@"|' "$HOME/.xserverrc"
fi

echo "== Done. Reboot to load the new UI."
read -p "Reboot now? [y/N] " a
[ "$a" = "y" ] && sudo reboot
