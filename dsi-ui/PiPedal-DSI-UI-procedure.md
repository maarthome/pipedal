# PiPedal on a 7" DSI screen

Procedure: build, install, start and stop PiPedal with the touchscreen interface

# 1. Purpose

This document describes how to build, install and run PiPedal with the customised web interface for the 7-inch DSI touchscreen (800 x 480) on a Raspberry Pi 5 with Raspberry Pi OS Trixie. The customised interface is part of the source in branch my_mods of github.com/maarthome/pipedal (based on PiPedal 2.0.111), so building that branch gives the touchscreen interface directly. PiPedal and the touchscreen kiosk (X + Chromium) are started and stopped by hand; nothing starts at boot.

The small-screen changes apply only when the browser window is shorter than 500 px, so the web interface on a phone or PC is not affected.

| **Script (repository root)** | **Purpose**                                                                                |
|------------------------------|--------------------------------------------------------------------------------------------|
| my_pipedal_install.sh        | Installs build tools, builds and installs PiPedal, sets up manual start and stop.          |
| pipedal-start.sh             | Starts the PiPedal services and then the touchscreen kiosk.                                |
| pipedal-stop.sh              | Stops the kiosk and the PiPedal services.                                                  |
| pipedal-run.sh               | Does the work for start and stop; also status, manual-mode, update-kiosk, autostart.       |
| pipedal-remove.sh            | Uninstalls PiPedal; --kiosk also removes the kiosk service; --purge-data deletes all data. |

# 2. Install, start and stop

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

- Switches off a console autologin from an earlier autostart kiosk setup, if present (saved as autologin.conf.off) and removes the startx block from ~/.bash_profile (backup ~/bash_profile.manual.bak).

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

# 3. What the customisation does

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

# 4. Changing the interface

**Procedure:** edit the files in ~/git/pipedal/vite/src, then in ~/git/pipedal/vite run NODE_OPTIONS=--max-old-space-size=2048 ./build.sh and sudo rm -rf /etc/pipedal/react/assets && sudo cp -r dist/\* /etc/pipedal/react/, test, and commit and push the source with git.

Do not run npm audit fix; it can change package versions and break the build. A full rebuild with ./my_pipedal_install.sh --continue also works, but takes longer.

# 5. Troubleshooting

| **Symptom**                                                | **Likely cause**                                                                      | **Action**                                                                                                                                                                                                                                    |
|------------------------------------------------------------|---------------------------------------------------------------------------------------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| Build error TS6133 (declared but never read)               | A leftover import or constant after an edit.                                          | Remove the unused import or constant named in the message and build again.                                                                                                                                                                    |
| Build is killed or runs out of memory                      | Not enough RAM.                                                                       | Close other programs or add swap, then continue with ./my_pipedal_install.sh --continue.                                                                                                                                                      |
| Screen still shows the old interface                       | Old files cached, or install did not run.                                             | Compare ls /etc/pipedal/react/assets/main-\*.js with curl -s http://localhost/ \| grep -o 'main-\[A-Za-z0-9\_-\]\*\\js'; then reboot.                                                                                                         |
| Source build fails: lv2.h or lv2/log/logger.h not found    | The lv2-dev package comes from the KXStudio repository and has no /usr/include/lv2.h. | Run my_pipedal_install.sh (it repairs this), or: sudo rm -f /usr/include/lv2/lv2plug.in/ns/ext /usr/include/lv2/lv2plug.in/ns/extensions, then sudo apt install --allow-downgrades lv2-dev=\<Debian version from apt-cache madison lv2-dev\>. |
| The interface only appears after a reload, or minutes late | Chromium was started before the PiPedal web server was up.                            | Use the current pipedal-run.sh (git pull): it starts the kiosk last. Then ./pipedal-stop.sh and ./pipedal-start.sh.                                                                                                                           |
| pipedal-stop.sh takes 90 seconds                           | Old kiosk service file without the short stop timeout.                                | ./pipedal-run.sh update-kiosk, then sudo systemctl reset-failed pipedal-kiosk.                                                                                                                                                                |
| Black screen after pipedal-start.sh                        | X or Chromium did not start.                                                          | journalctl -u pipedal-kiosk -n 40 --no-pager shows the reason.                                                                                                                                                                                |
| Layout of the DSI screen appears in a PC browser           | The small-screen layout applies to any window shorter than 500 px.                    | Make the browser window taller.                                                                                                                                                                                                               |
| npm install is slow                                        | Slow network.                                                                         | Wait; it can take several minutes.                                                                                                                                                                                                            |

## Known points that are not caused by this customisation

- Plugin graphics that do not refresh while a control is moved also occur in the web interface on a PC, so they originate from PiPedal or the plugin itself.
