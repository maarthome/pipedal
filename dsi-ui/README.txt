PiPedal with the 7" DSI touchscreen interface (branch my_mods)
==============================================================

The touchscreen changes are part of the source of this branch (vite/src).
All scripts are in the repository root:

  ./my_pipedal_install.sh   build + install PiPedal, set up manual start/stop
  ./pipedal-start.sh        start PiPedal and the touchscreen kiosk
  ./pipedal-stop.sh         stop the kiosk and PiPedal
  ./pipedal-run.sh status   show what is running (also: manual-mode, update-kiosk, autostart)
  ./pipedal-remove.sh       uninstall

Nothing starts at boot. Full description: PiPedal-DSI-UI-procedure.md (or .docx) in this folder.
