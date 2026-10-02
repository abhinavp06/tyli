# Unreleased

Work that is done but not yet part of a version. The newest entries come first.

### File Sync UI built: bridge, paged file list, every screen, tests (2026-10-02, branch `abhinavp06/FILE_SCANNER_INIT`)

**Summary:** Phases 1 and 2 of `features/file_sync_ui.md`. TYLI now drives hoardor File Sync v1 end to end: configure categories and folders, sync a section in the background with progress and Cancel, browse each folder's files (paged from SQLite), read what the last sync did, handle offline folders and held removals, and edit every hoardor setting. Uses hoardor's existing API only; hoardor is unchanged.

**Added**
- `src/library_bridge.hpp/.cpp`: `LibraryBridge`, the QML singleton `Library`.
  - Owns the UI thread's hoardor `Database` + `Library` and a `master::SyncWorker`. Worker callbacks are forwarded to the UI thread with queued `QMetaObject::invokeMethod` calls.
  - `syncing` comes from polling `SyncWorker::idle()` every 100 ms while a sync runs, because the "finished" callback runs before the worker clears its running flag.
  - `addFolder` / `relocateFolder` run on Qt's thread pool (`QtConcurrent::run`) with their own short-lived connection, and report back through `folderOperationFinished`. The `QFutureWatcher` is a child of the bridge, so no result arrives after the bridge is gone.
  - Exposes categories, folders, settings (camelCase map), `settingLimits` (hoardor's limits table), `fileKinds`, `syncProgress`, `lastReport`, `categoryReports`, `folderOperationRunning`, and the open state (`ready`, `openError`, `databasePath`).
  - The destructor resets the SyncWorker first, which cancels and joins hoardor's thread before anything else goes away.
- `src/folder_files_model.hpp/.cpp`: `FolderFilesModel`, a `QAbstractListModel` that pages `Library::entries(root, after_id, 200)` through `canFetchMore` / `fetchMore` (keyset paging, so each page costs the same at any depth). Roles: `relativePath`, `size`, `kind`, `unsettled`, `mtime`. `reload()` after a sync.
- `qml/`: `Main.qml` (category tabs + Settings button, error screen, message dialog), `SectionView.qml`, `SyncDetailsDialog.qml`, `SettingsView.qml`, `FolderSettings.qml`, `CategorySettings.qml`, `ScanningSettings.qml`, `SyncSettings.qml`, `Format.js` (number, size, and "5 min ago" formatting).
- `tests/`: `CMakeLists.txt`, `test_support.hpp` (media trees, waiting for background work), and three Qt Test suites run by `ctest` offscreen:
  - `tst_library_bridge` (20 tests): default categories; adding a folder (background, marker written, `file://` URL or path, without marker); missing / duplicate / overlapping folders; Unicode paths (`Música ✓/Ünïcødé/Café.flac`); sync counts (added, modified, removed, unchanged) and `syncing` going true then false; global sync; a repeated request ignored; cancel mid-sync (nothing removed, reported as cancelled); unplugged folder → offline with its files kept; relocate (and refused to a different folder); held removals then apply; scan errors for an unreadable folder (skipped as root); folder category / case-sensitivity / removal (files on disk untouched); categories CRUD and errors (duplicate, unknown kind, in use); settings round trip, validation refusals with hoardor's message, persistence across restarts; sync at startup; an unopenable database reported instead of a crash; destroying the bridge mid-sync returns in under 2 s.
  - `tst_folder_files_model` (6 tests): 1,000 files in five pages of 200 with no gaps or duplicates; roles; `reload` after a sync sees new files; switching folders; an offline folder still lists its files; an unknown folder is empty.
  - `tst_qml_smoke`: loads `Main.qml` over a seeded library (an offline folder, held removals), visits every section and every Settings sub-tab, checks the file list fills, checks a new category appears as a tab, fails on any QML warning, and saves screenshots to `build/screenshots/`.

**Changed**
- `CMakeLists.txt`: the QML module `Tyli` is now the static library `tyli_ui` (bridge + screens, build output in `build/qml/Tyli`); the app links `tyli_ui` + `tyli_uiplugin`. Adds `Qt6::Concurrent`, and `TYLI_BUILD_TESTS` (default ON when TYLI is the top-level project) for `tests/` (`Qt6::Test`).
- `src/main.cpp`: imports the static QML plugin (`Q_IMPORT_QML_PLUGIN(TyliPlugin)`), names the app "TYLI" (so the data folder is `%APPDATA%\TYLI` / `~/.local/share/TYLI`), and constructs the bridge on `LibraryBridge::defaultDataDir()` (`TYLI_DATA_DIR` overrides it) before the QML engine.
- `tools/vnc_session.sh`: opens TYLI at the virtual screen's size (`-qwindowgeometry`), because there's no window manager on Xvfb to maximize it.
- `DOCUMENTATION/application/features/file_sync_ui.md`: status, the bridge as built, §6 as built, phases.
- `CLAUDE.md`: `tests/` and the `tyli_ui` module in the layout.

**Design decisions and trade-offs**
- **The bridge returns plain `QVariantList` / `QVariantMap` for categories, folders, and reports**, and a real `QAbstractListModel` only for files. Categories and folders are a handful of rows; files can be 500k. One model class, not one per list.
- **Category summaries come from per-folder reports** (`categoryReports`), grouped by each folder's current category, instead of only the last global report. A Music sync followed by a Movies sync leaves both sections with their own summary.
- **Settings ranges come from hoardor** (`settingLimits`), so the spin boxes and hoardor's validation can't disagree. A save that hoardor refuses shows hoardor's message.
- **Settings is a separate button, not the last tab.** With a static tab after a `Repeater` in the same `TabBar`, where new category tabs land depends on how the container orders repeated items. A separate button keeps Settings fixed at the right, whatever is added.
- **Rejected:** `QQmlContext::setContextProperty` for the bridge (untyped, slower lookups, invisible to tooling) in favor of `QML_SINGLETON` with a static `create()` that returns the instance `main()` owns.

**Bugs found by the new tests (fixed before this entry)**
- The category sections were blank: a `Repeater` delegate that declares `required` properties stops getting `modelData` implicitly. `Main.qml` now declares `required property var modelData` on the delegate.

**Tests**
- `ctest`: 3/3 suites pass (about 10 s). Build clean with `-Wall -Wextra -Wpedantic`. The app starts with no QML warnings.

**Known limitations / follow-ups**
- Sync reports live for the session only (hoardor keeps scan errors and held removals, but not the counts).
- The file list is in scan order. Sorting and media listing are the next feature.
- `FolderDialog` isn't exercised by the tests; on the VM it's Qt Quick's own dialog (no native one under Xvfb).
- The unreadable-folder test skips when run as root; run `tst_library_bridge` as an unprivileged user to cover it (as for hoardor).
- Phase 3: the user builds on Windows and syncs real external drives.

### Live UI view: `vnc_session.sh --watch` restarts TYLI after every rebuild (2026-10-02, branch `abhinavp06/FILE_SCANNER_INIT`)

**Summary:** The user works from Windows over SSH and wants to watch the UI change as it's built. `tools/vnc_session.sh --watch` keeps Xvfb and x11vnc running and restarts TYLI whenever its binary changes or it exits. The VNC viewer stays connected through restarts.

**Changed**
- `tools/vnc_session.sh`:
  - new `--watch` first argument: polls the binary's mtime once a second, waits until it has stopped changing (so the linker is done), then restarts TYLI. Also restarts TYLI if it's closed or crashes.
  - TYLI is started as a plain command, not through a shell function. A backgrounded function runs in a subshell, so `$!` would be the subshell and killing it left the old TYLI running (found while testing: two instances after a restart).
  - Without `--watch`, behavior is unchanged.
- `CLAUDE.md`: the live-view command (run detached with `setsid nohup` so it outlives an agent session), and how to connect from Windows (TigerVNC or RealVNC Viewer at `localhost:5900`) vs macOS (port 5901).

**Design decisions and trade-offs**
- **Restart, not QML hot reload.** The QML is compiled into the app's resources, and the bridge is C++ anyway, so most changes need a rebuild. A restart after each build is simple and shows C++ and QML changes alike. Loading QML from the source tree at runtime was rejected: a second loading path just for development.
- **mtime polling, not `inotifywait`.** No extra package, and one `stat` a second costs nothing.

**Tests**
- On the VM: x11vnc listens only on `127.0.0.1:5900` and `[::1]:5900`. Touching `build/tyli` restarts TYLI within about 2 s, and exactly one TYLI process is left afterwards.

**Known limitations / follow-ups**
- A restart loses view state (the open tab, scroll position). Fine for now.

### File Sync UI design approved; build not started (2026-10-02, branch `abhinavp06/FILE_SCANNER_INIT`)

**Summary:** The user approved building the File Sync UI as designed (`features/file_sync_ui.md`) after seeing the skeleton over VNC. The first build step (restructuring `CMakeLists.txt` into a `tyli_ui` QML module) was interrupted when the session ended, before any file was written. The planned structure is recorded in the feature doc, §6.

**Changed**
- `DOCUMENTATION/application/features/file_sync_ui.md`: status Approved; new §6 "Build structure (planned)".

**Next**
1. Phase 1: `LibraryBridge` + `FolderFilesModel` + Qt tests.
2. Phase 2: QML screens + smoke test with screenshots.
3. Restart `tools/vnc_session.sh` so the user can click through it (password in `~/.vnc/tyli_passwd` on the VM).
4. The user tests, then both repos' `abhinavp06/FILE_SCANNER_INIT` merge to `master` (hoardor `v0.1.0`, TYLI `v0.1.0`).
5. Media listing starts on new branches in both repos.

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
- **Update (same day):** the script now uses a VNC password when `~/.vnc/tyli_passwd` exists (created with `x11vnc -storepasswd`), because macOS Screen Sharing insists on one. It also keeps other users on the VM out. The password lives only on the VM, never in the repo.

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
