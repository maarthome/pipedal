# PiPedal on a 7" DSI screen

Procedure: install, patch and build the customised web interface

# 1. Purpose

This document describes how to rebuild and install the customised PiPedal web interface for the 7-inch DSI touchscreen (800 x 480) of the Raspberry Pi 5 running Raspberry Pi OS Trixie. The Pi starts Xorg through startx and shows PiPedal in a Chromium kiosk window (http://localhost). All changes are kept in one patch file, and one script rebuilds and installs everything, so the result can be reproduced at any time.

**Version:** PiPedal 2.0.110, source commit a0d9766e331a696b8450a74d8cc85c1c093a83d7. The changes apply only when the browser window is shorter than 500 px, so the web interface on a phone or PC is not affected.

# 1b. Two ways to get the customised interface

**A. Your fork (recommended):** the changes are already part of the source in branch my_mods of github.com/maarthome/pipedal (based on PiPedal 2.0.111). Building that branch gives the touchscreen interface directly. Section 1c describes this route.

**B. A plain upstream PiPedal:** use the patch and dsi-ui-install.sh from the dsi-ui folder (sections 5 to 9 below). They build the web interface from the upstream sources at the recorded commit (2.0.110) and install it over an existing PiPedal.

Do not run dsi-ui-install.sh on a PiPedal that was built from my_mods: it would replace the current interface with the older 2.0.110 one.

**Changing the interface on my_mods:** edit the files in ~/git/pipedal/vite/src, then in ~/git/pipedal/vite run NODE_OPTIONS=--max-old-space-size=2048 ./build.sh and sudo rm -rf /etc/pipedal/react/assets && sudo cp -r dist/\* /etc/pipedal/react/, test, and commit and push the source with git. The patch file in dsi-ui is then out of date; it is only a snapshot for route B.

# 1c. Route A: install, start and stop (fork my_mods)

## Install

```bash
mkdir -p ~/git && cd ~/git
git clone --recurse-submodules https://github.com/maarthome/pipedal
cd pipedal
./my_pipedal_install.sh
```

my_pipedal_install.sh installs the build tools and libraries, repairs a broken KXStudio lv2-dev if needed, builds PiPedal (about 420 compile steps; ./my_pipedal_install.sh --continue resumes an interrupted build), installs the .deb package and then asks whether to set up manual start and stop of the touchscreen kiosk (pipedal-run.sh manual-mode). Node.js 22 or later is required.

## Start and stop by hand (nothing starts at boot)

```bash
cd ~/git/pipedal
./pipedal-start.sh # PiPedal services, then X + Chromium on the touchscreen
./pipedal-stop.sh # Chromium + X, then the PiPedal services
./pipedal-run.sh status # services (running / at boot), web server, installed UI, kiosk
```

Starting takes about 30 seconds, because PiPedal scans all LV2 plugins before its web server answers; the screen is started after that. The scripts are used from the repository folder and can be run over SSH. When the kiosk stops, the login prompt returns on the touchscreen.

## What manual-mode sets up (run once)

- Installs xserver-xorg, xinit, x11-xserver-utils and chromium; creates ~/.xserverrc (no blanking, no mouse pointer) if it does not exist.

- Disables the PiPedal services at boot (the list is remembered in ~/.pipedal-autostart; ./pipedal-run.sh autostart on restores it).

- Switches off a console autologin made by kiosk-setup.sh (saved as autologin.conf.off) and removes the startx block from ~/.bash_profile (backup ~/bash_profile.manual.bak).

- Creates /etc/systemd/system/pipedal-kiosk.service (startx with Chromium in kiosk mode on tty1). It is never enabled, so it only runs after a start.

## Other commands

```bash
./pipedal-run.sh restart # stop, then start everything
./pipedal-run.sh restart-kiosk # only the screen
./pipedal-run.sh restart-pipedal # only the PiPedal services
./pipedal-run.sh update-kiosk # rewrite the kiosk service file (stop first)
./pipedal-run.sh autostart on|off # PiPedal services at boot
./pipedal-remove.sh # uninstall PiPedal (--kiosk, --purge-data: see the script)
```

Extra Chromium options can be written into the kiosk service with CHROMIUM_FLAGS="..." ./pipedal-run.sh update-kiosk; running update-kiosk without it removes them.

The other way to use PiPedal is from any browser on the same network: http://raspberrypi/ (the host name of the Pi) while the services run. On first use choose the audio device in the Settings menu.

# 2. What the customisation does

## Chain overview (main screen)

- The whole plugin chain is fitted to the screen automatically, both in width and height.

- Plugin frames are twice as large (72 px), with the picture inside enlarged as well; the distance between plugins is shortened to 6 px.

- A single tap on a plugin selects it. A double tap opens its edit screen; on an empty slot it opens the plugin selection window.

- A double tap on the background shows the whole chain again. A two-finger pinch zooms the chain.

- The full-screen button toggles between the whole chain (with the chain name) and a zoom that shows all plugins vertically (width may be cut off).

- The chain name is shown in large yellow letters above the thin line, only in the whole-chain view.

- A bottom strip holds the buttons Add (+), Load, MIDI bindings and Snapshots.

## Plugin edit screen

- Opens full screen; only vertical scrolling.

- Top bar: back arrow, bypass switch, plugin name, info, UI switch, Delete, preset selector and Load, all visible and selectable.

## Snapshots screen

- Full screen with the chain name in yellow at the top (the word Snapshots is removed) and six tiles of equal size.

## Other

- The XRuns / CPU / temperature text no longer blocks touches on the buttons beneath it.

- The mouse pointer is hidden on the kiosk (option -nocursor in ~/.xserverrc).

## Files changed in the PiPedal source (folder vite/src/pipedal)

AppThemed.tsx, JackStatusView.tsx, MainPage.tsx, PedalboardView.tsx, PluginControlView.tsx, SnapshotDialog.tsx, SnapshotPanel.tsx, SplitControlView.tsx.

# 3. What is on the Pi

| **Location**            | **Contents**                                                                                                          |
|-------------------------|-----------------------------------------------------------------------------------------------------------------------|
| ~/pipedal-dsi-ui.patch  | All source changes as one patch (made with git diff).                                                                 |
| ~/pipedal-dsi-ui.commit | The PiPedal commit the patch belongs to.                                                                              |
| ~/dsi-ui-install.sh     | Script that rebuilds and installs everything.                                                                         |
| ~/react.stock           | Copy of the original web interface, made by the script on its first run. Used for restoring.                          |
| /etc/pipedal/react      | The installed web interface that the kiosk displays.                                                                  |
| ~/pipedal               | Original working clone in which the changes were developed. Not needed, and absent on a Pi that only used the script. |
| ~/pipedal-build         | Clone created by the script on every run, with the patch applied. Use this folder for further changes.                |

**Keep a copy on your PC:** pipedal-dsi-ui.patch, pipedal-dsi-ui.commit and dsi-ui-install.sh. Example, run on the PC: scp maart@\<ip of the Pi\>:pipedal-dsi-ui.patch .

# 4. Requirements

- Raspberry Pi OS Trixie with PiPedal installed; its web interface lives in /etc/pipedal/react.

- User maart with sudo rights; the commands are run as maart, not as root (sudo only where shown).

- Internet access: the script downloads the source (about 1 GB) and the npm packages.

- Packages git, nodejs (version 20.x) and npm; the script installs them with apt.

- 4 GB RAM plus swap is enough; the build takes a few minutes.

# 5. Rebuild and install with the script

**Original interface:** on the first run the script saves the untouched interface from /etc/pipedal/react as ~/react.stock. This only works while PiPedal still has its original interface. If ~/react.stock is missing and the installed interface is already modified, the script stops with an error; restore the original first (section 8).

To check ~/react.stock at any time (it must print nothing):

```bash
grep -l pipedal-toggle-fit ~/react.stock/assets/*.js
```

Then start the script:

```bash
~/dsi-ui-install.sh
```

The script performs these steps:

1.  **Packages:** installs git, nodejs and npm (nothing happens if they are already present).

2.  **Source:** clones PiPedal into ~/pipedal-build, checks out the recorded commit and applies the patch. If the patch does not fit, it stops here before anything is installed.

3.  **Build:** npm install, then build.sh (TypeScript check, Vite build, gzip of the .js files). Success ends with a list of files and the text built in ... ms.

4.  **Install:** removes /etc/pipedal/react/assets and copies the new files into /etc/pipedal/react.

5.  **Mouse pointer:** adds -nocursor to ~/.xserverrc (a backup is kept in ~/xserverrc.bak).

6.  **Reboot:** the script asks whether to reboot. The kiosk needs a minute or two to come back; the UI is static web files, so PiPedal itself does not have to be restarted.

**Check afterwards:** ls /etc/pipedal/react/assets/main-\*.js shows the installed build. The screen should show the whole chain, the bottom strip and no mouse pointer.

# 6. Manual procedure (what the script does)

Use this if you want to run the steps one by one.

```bash
sudo apt install -y git nodejs npm
git clone https://github.com/rerdavies/pipedal ~/pipedal-build
cd ~/pipedal-build
git checkout $(cat ~/pipedal-dsi-ui.commit)
git apply --check ~/pipedal-dsi-ui.patch
git apply ~/pipedal-dsi-ui.patch
cd vite
npm install
NODE_OPTIONS=--max-old-space-size=2048 ./build.sh
sudo rm -rf /etc/pipedal/react/assets
sudo cp -r dist/* /etc/pipedal/react/
sudo reboot
```

Do not run npm audit fix; it can change package versions and break the build.

# 7. Saving further changes

The install script builds in ~/pipedal-build (a fresh clone with the patch applied). Make further changes there, in ~/pipedal-build/vite/src (rebuild with the commands from section 6). After you have tested the result, record the new state so that the script reproduces it:

```bash
cd ~/pipedal-build
git rev-parse HEAD > ~/pipedal-dsi-ui.commit
git diff -- vite/src > ~/pipedal-dsi-ui.patch
git diff --stat -- vite/src
```

Always use git diff -- vite/src so the patch does not contain package-lock.json, which npm install changes.

# 8. Going back to the original interface

```bash
sudo rm -rf /etc/pipedal/react
sudo cp -a ~/react.stock /etc/pipedal/react
sudo reboot
```

## If ~/react.stock is lost

Build the original interface from source at the recorded commit and save it as ~/react.stock (nothing in /etc/pipedal/react changes):

```bash
git clone https://github.com/rerdavies/pipedal ~/pipedal-stock
cd ~/pipedal-stock
git checkout $(cat ~/pipedal-dsi-ui.commit)
cd vite
npm install
NODE_OPTIONS=--max-old-space-size=2048 ./build.sh
sudo cp -a /etc/pipedal/react ~/react.stock
sudo rm -rf ~/react.stock/assets
sudo cp -r dist/assets ~/react.stock/assets
sudo cp dist/index.html ~/react.stock/index.html
```

Then reboot (or restart Chromium) so the kiosk loads the original interface. The files in ~/react.stock belong to root, so use sudo for every command that changes them.

To undo only the hidden mouse pointer, restore the backup: cp ~/xserverrc.bak ~/.xserverrc, then reboot.

# 9. After a PiPedal upgrade

A PiPedal upgrade replaces the web interface in /etc/pipedal/react with the standard one. There are two ways to get the customised version back:

- **Same PiPedal version as the recorded commit:** run ~/dsi-ui-install.sh again.

- **Newer PiPedal version:** do not run the script blindly, because the old interface may not match the new server. Clone the new version, run git apply --3way ~/pipedal-dsi-ui.patch in the clone, resolve any conflicts, build and install as in section 6. Then record the new state as in section 7.

# 10. Kiosk that starts at boot (alternative, kiosk-setup.sh)

**Note:** this is the autostart variant. Do not combine it with the manual start and stop of section 1c; pipedal-run.sh manual-mode switches it off again.

The install script assumes the kiosk already runs. On a fresh Raspberry Pi OS (Trixie) with PiPedal installed, run ~/kiosk-setup.sh once, as maart, before the install script. The 7-inch DSI screen itself needs no extra setup; it was detected automatically (output DSI-2, touch controller ft5x06).

What the kiosk consists of:

- **Console autologin:** /etc/systemd/system/getty@tty1.service.d/autologin.conf logs maart in on tty1 without a password.

- **~/.bash_profile:** first loads ~/.bashrc (aliases and functions; a login shell skips it otherwise), then on tty1 (not over SSH) it runs startx with Chromium in --kiosk mode on http://localhost (--incognito, --no-sandbox, no error dialogs or info bars).

- **~/.xserverrc:** starts X with -s 0 -dpms (no screen blanking) and -nocursor (hidden mouse pointer).

- **Packages:** xserver-xorg, xinit, x11-xserver-utils and chromium.

Run it with:

```bash
chmod +x ~/kiosk-setup.sh
~/kiosk-setup.sh
sudo reboot
```

Existing ~/.xserverrc and ~/.bash_profile are saved as ~/xserverrc.bak and ~/bash_profile.bak. To stop the kiosk from starting, rename ~/.bash_profile (over SSH) and reboot.

Full script:

```bash
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
```

# 11. Troubleshooting

| **Symptom**                                                | **Likely cause**                                                                      | **Action**                                                                                                                                                                                                                                    |
|------------------------------------------------------------|---------------------------------------------------------------------------------------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| git apply --check fails                                    | The source is not the recorded version.                                               | Check that ~/pipedal-dsi-ui.commit is used (git checkout) and that the patch file is complete.                                                                                                                                                |
| Build error TS6133 (declared but never read)               | A leftover import or constant after an edit.                                          | Remove the unused import or constant named in the message and build again.                                                                                                                                                                    |
| Build is killed or runs out of memory                      | Not enough RAM.                                                                       | Use NODE_OPTIONS=--max-old-space-size=2048, close other programs, or build on another computer and copy the dist folder.                                                                                                                      |
| Screen still shows the old interface                       | Old files cached, or install did not run.                                             | Compare ls /etc/pipedal/react/assets/main-\*.js with curl -s http://localhost/ \| grep -o 'main-\[A-Za-z0-9\_-\]\*\\js'; then reboot.                                                                                                         |
| Source build fails: lv2.h or lv2/log/logger.h not found    | The lv2-dev package comes from the KXStudio repository and has no /usr/include/lv2.h. | Run my_pipedal_install.sh (it repairs this), or: sudo rm -f /usr/include/lv2/lv2plug.in/ns/ext /usr/include/lv2/lv2plug.in/ns/extensions, then sudo apt install --allow-downgrades lv2-dev=\<Debian version from apt-cache madison lv2-dev\>. |
| The interface only appears after a reload, or minutes late | Chromium was started before the PiPedal web server was up.                            | Use the current pipedal-run.sh (git pull): it starts the kiosk last. Then ./pipedal-stop.sh and ./pipedal-start.sh.                                                                                                                           |
| pipedal-stop.sh takes 90 seconds                           | Old kiosk service file without the short stop timeout.                                | ./pipedal-run.sh update-kiosk, then sudo systemctl reset-failed pipedal-kiosk.                                                                                                                                                                |
| Black screen after pipedal-start.sh                        | X or Chromium did not start.                                                          | journalctl -u pipedal-kiosk -n 40 --no-pager shows the reason.                                                                                                                                                                                |
| Autostart kiosk (section 10) does not start                | Problem with ~/.xserverrc.                                                            | Over SSH: cp ~/xserverrc.bak ~/.xserverrc and reboot.                                                                                                                                                                                         |
| Buttons need a tap above the icon                          | The status text covered them.                                                         | Included in the patch (JackStatusView.tsx, pointer-events none). Check that the patch was applied.                                                                                                                                            |
| Layout of the DSI screen appears in a PC browser           | The small-screen layout applies to any window shorter than 500 px.                    | Make the browser window taller.                                                                                                                                                                                                               |
| npm install is slow                                        | Slow network.                                                                         | Wait; it can take several minutes.                                                                                                                                                                                                            |

## Known points that are not caused by this customisation

- Plugin graphics that do not refresh while a control is moved also occur in the web interface on a PC, so they originate from PiPedal or the plugin itself.

# Appendix: the script ~/dsi-ui-install.sh

Create the file with a text editor or with cat, then make it executable with chmod +x ~/dsi-ui-install.sh. If pasting into the terminal garbles lines, paste in smaller parts and check with bash -n ~/dsi-ui-install.sh.

```bash
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
```
