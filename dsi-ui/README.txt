On the my_mods branch the DSI changes are already part of the source (vite/src).
Install:      ../my_pipedal_install.sh
Start / stop: ../pipedal-start.sh  ../pipedal-stop.sh   (nothing starts at boot)
Other:        ../pipedal-run.sh status|manual-mode|update-kiosk ...   ../pipedal-remove.sh
Full description: PiPedal-DSI-UI-procedure.docx (section 1c).

The files below are only needed to add the changes to a plain upstream PiPedal 2.0.110
(patch + dsi-ui-install.sh), or for a kiosk that starts at boot (kiosk-setup.sh).

Contents
  pipedal-dsi-ui.patch            all source changes (git patch, vite/src only)
  pipedal-dsi-ui.commit           PiPedal commit the patch belongs to
  dsi-ui-install.sh               rebuilds the web UI from source and installs it
  ../pipedal-run.sh               (repo root) start/stop/restart/status of PiPedal and the kiosk
  ../pipedal-remove.sh            (repo root) remove PiPedal; --kiosk undoes the kiosk setup; --purge-data deletes all data
  kiosk-setup.sh                  Chromium kiosk (autologin, startx, hidden pointer)
  PiPedal-DSI-UI-procedure.docx   full description of the procedure

Order on a fresh Raspberry Pi 5 (Raspberry Pi OS Trixie, user maart)
  1. Install PiPedal 2.0.110 as usual.
  2. Copy all files except the .docx to the home folder of the user (~):
        scp pipedal-dsi-ui.* dsi-ui-install.sh kiosk-setup.sh maart@<ip of the Pi>:
  3. Only if the kiosk is not set up yet:   chmod +x ~/kiosk-setup.sh && ~/kiosk-setup.sh
  4. (nothing to do) On its first run the script saves the untouched interface as
     ~/react.stock. Do this on a Pi that still has the original PiPedal interface.
     If ~/react.stock is lost later, see section 8 of the document.
  5. chmod +x ~/dsi-ui-install.sh && ~/dsi-ui-install.sh
  6. Reboot.

Back to the original interface: see section 8 of the document.
Run the scripts as the normal user, not as root. Do not run "npm audit fix".
