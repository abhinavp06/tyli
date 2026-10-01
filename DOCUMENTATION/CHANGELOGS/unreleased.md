# Unreleased

Work that is done but not yet part of a version. The newest entries come first.

### Seeing the UI from the VM: Xvfb + x11vnc on localhost, over SSH (2026-10-01, branch `abhinavp06/FILE_SCANNER_INIT`)

**Summary:** The VM is a remote KVM server reached over SSH, with no display and no firewall. `tools/vnc_session.sh` runs TYLI on a virtual X display and shares it over VNC bound to `127.0.0.1` only. The user reaches it by SSH port forwarding, so nothing is exposed to the network.

**Added**
- `tools/vnc_session.sh`:
  - starts `Xvfb :99` (`-nolisten tcp`)
  - starts `x11vnc -localhost -rfbport 5900`
  - runs TYLI on the `xcb` backend with Qt Quick's software renderer
  - cleans everything up on exit
  - port, display, screen size, and binary can be changed with environment variables
- `CLAUDE.md`: how to use it, and the warning below.

**Environment** (outside the repo)
- apt packages `xvfb` and `x11vnc`.

**Fixed / incident**
- The first attempt used Qt's built-in VNC backend (`QT_QPA_PLATFORM=vnc:...:addr=127.0.0.1`). Qt **ignored the address** and listened on `*:5900`, and this VM has no firewall (`ufw` inactive, `INPUT ACCEPT`). For about 5 seconds, an unauthenticated VNC view of the skeleton window (one line of text, no user data) was reachable on the public IP. That backend must not be used here, and `CLAUDE.md` now says so.
- **Verified:** with the new script, `x11vnc` listens only on `127.0.0.1:5900` and `[::1]:5900`, and nothing is left running afterwards.

**Design decisions and trade-offs**
- **Rejected:** SSH X11 forwarding (`ssh -X`). It needs an X server on Windows and is slow over the internet for Qt Quick.
- **Rejected:** a remote desktop environment. Too heavy for this.
- **Chosen:** VNC over an SSH tunnel. It works with any VNC viewer, and the only port involved is SSH's.

**Known limitations / follow-ups**
- The VM can't see the user's external HDDs. Real-drive testing needs the Windows build.
- `-nopw` is acceptable only because of the localhost binding. A VNC password can be added if the VM ever gets other users.

### Restart: Qt 6.12 skeleton on hoardor, project docs, File Sync UI design (2026-10-01, branch `abhinavp06/FILE_SCANNER_INIT`)

**Summary:** TYLI restarts on `master` (reset by the user). This branch builds against hoardor from the sibling workspace folder, sets up the project docs (modeled on hoardor's), and designs the first feature: the File Sync UI. The UI design awaits the user's approval.

**Added**
- `CMakeLists.txt`:
  - finds Qt 6.12 (Quick, QuickControls2)
  - adds `../hoardor` with `add_subdirectory` (path configurable through `TYLI_HOARDOR_DIR`) **before** `qt_standard_project_setup()`, so Qt's AUTOMOC never runs on hoardor
  - defines `qt_add_executable(tyli)` + `qt_add_qml_module(URI Tyli)`
  - warnings on, `WIN32_EXECUTABLE`
- `src/main.cpp`: the skeleton. Fusion style, and the QML module loaded from `Tyli`. It opens an in-memory hoardor library and shows its categories, proving the link works.
- `qml/Main.qml`: a skeleton window.
- `CLAUDE.md`: purpose, workspace, reading order, non-negotiables (UI thread never blocks, paged models, thin bridge), layout, build (VM and Windows), Qt-style naming, workflow, Definition of done.
- `DOCUMENTATION/application/ARCHITECTURE.md`: layers (QML → bridge → hoardor), threads, data location, look, testing, decision log.
- `DOCUMENTATION/application/features/file_sync_ui.md` (Proposed): screens with wireframes, the bridge API (`LibraryBridge`, `FolderFilesModel`), edge cases, tests, phases.
- `DOCUMENTATION/CHANGELOGS/README.md` and this file.

**Changed**
- `.gitignore`: `build*/` added to the Visual Studio entries.

**Environment** (outside the repo)
- Qt 6.12.0 for `linux_gcc_64` was installed with aqtinstall into `/opt/Qt` (1.7 GB, plus the `qttasktree` module, which a Qt 6.12 QML plugin depends on), along with the OpenGL, xkbcommon, and xcb development packages from apt.

**Design decisions and trade-offs**
- **Qt 6.12:** the newest release, so a new project doesn't start with a migration. Ubuntu's Qt 6.4 was rejected: it's old and unsupported, and it wouldn't match Windows.
- **`add_subdirectory(../hoardor)`** (the user's choice): hoardor changes show up immediately during side-by-side development.
- **Qt-style naming in TYLI:** almost everything is exposed to QML, where `camelCase` is the convention.

**Tests**
- Configure is clean (no warnings). The build passes. `QT_QPA_PLATFORM=offscreen ./build/tyli` starts and keeps running with no QML errors. Real tests come with phase 1 of the File Sync UI.

**Known limitations / follow-ups**
- The UI design awaits the user's approval. Then: phase 1 (bridge + tests), phase 2 (QML screens + smoke test with screenshots), phase 3 (Windows check by the user).
