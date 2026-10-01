# Unreleased

Work that is done but not yet part of a version. The newest entries come first.

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
