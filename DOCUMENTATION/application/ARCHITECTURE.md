# TYLI Architecture

This is a living document. It records TYLI's system-wide decisions and why they were made. The engine's architecture is in `../hoardor/DOCUMENTATION/application/ARCHITECTURE.md`, and §1 there ("hoardor vs TYLI") is binding for both repositories.

Status markers: **Decided** means agreed with the user. **Proposed** means it still needs the user's approval or a measurement.

## 1. Layers (Decided)

```
QML views (qml/)                  what the user sees; no business logic
      │  properties, invokables, signals, list models
C++ bridge (src/)                 QObjects and QAbstractListModels: converts hoardor types to Qt types,
      │                           and moves hoardor's background callbacks onto the UI thread
hoardor (../hoardor, add_subdirectory)   all real work: SQLite, scanning, sync, (later) metadata and playback
```

- **The bridge is thin.** If a view needs something hoardor can't provide, hoardor gets a feature first.
- **hoardor never sees Qt.** It's added to the build before Qt's project setup, so Qt's code generator (AUTOMOC) never runs on it.

## 2. Threads (Decided for sync; Proposed for the rest)

| Work | Runs on | How results reach the UI |
|---|---|---|
| Reading the library (categories, folders, file pages, settings) | UI thread, on the UI's own hoardor `Database` connection | Directly. It never touches a media drive, and it's paged |
| Config changes that don't touch a drive (rename a category, change settings) | UI thread | Directly |
| Config changes that touch a drive (add or relocate a folder: marker, case probe) | A Qt thread-pool task with a short-lived hoardor connection | A queued signal |
| Sync | `hoardor::master::SyncWorker` (hoardor's own thread) | Callbacks run on the worker. The bridge forwards them with `QMetaObject::invokeMethod(..., Qt::QueuedConnection)` |

Why: the UI must never freeze. A sleeping HDD takes 5–10 s to spin up, and anything that touches it must not run on the UI thread.

## 3. Data location (Proposed)

- **The library database** is `QStandardPaths::AppDataLocation/library.db`: on Windows `%APPDATA%\TYLI\library.db`, on Linux `~/.local/share/TYLI/library.db`. That's always internal storage, never a media drive (hoardor ARCHITECTURE §3).
- **Override:** the environment variable `TYLI_DATA_DIR` changes the folder, for tests and for trying things without touching the real library.

## 4. Look (Decided, 2026-10-02)

- **OLED black with minimal green** (`features/theme.md`): pure `#000000` everywhere, green only as text, thin lines, and small glyphs; IBM Plex Mono bundled; our own stroke icons; no popups (results go to a status line, everything else is inline).
- **Qt Quick Controls "Basic" style, restyled by TYLI's own controls** (`qml/controls/`, built on `QtQuick.Templates`). Every color comes from `qml/Theme.qml`.
- **TYLI is always dark;** it doesn't follow the OS's light mode.
- **UI changes start as HTML mockups** in `DOCUMENTATION/mockups/`, reviewed by the user before any QML is written.

## 5. Testing (Proposed)

- **Qt Test** (`tests/`, run by `ctest`) covers:
  - bridge tests against a temporary data folder (`TYLI_DATA_DIR`)
  - list-model paging tests
  - a QML smoke test that loads every screen offscreen, fails on any QML warning, and saves screenshots to the build folder for review
- **The VM has no display:** tests and screenshots use `QT_QPA_PLATFORM=offscreen`. Real interactive testing happens on the user's Windows machine.

## Decision log

| Date | Decision |
|---|---|
| 2026-10-01 | TYLI's `master` was reset by the user. TYLI restarts on `master`; older branches are left alone. New branches are based on `master`. |
| 2026-10-01 | Qt 6.12 (the latest release). On the Linux VM it's installed with aqtinstall into `/opt/Qt` (plus `qttasktree`, which a QML plugin in 6.12 depends on). On Windows: MSVC 2022 64-bit. |
| 2026-10-01 | TYLI consumes hoardor with `add_subdirectory(../hoardor)` (path configurable through `TYLI_HOARDOR_DIR`) for side-by-side development. A pinned hoardor tag through `FetchContent` can replace it later. |
| 2026-10-01 | C++ naming in TYLI follows Qt style (`camelCase` functions, properties, and signals), because almost everything is exposed to QML. hoardor keeps `snake_case`. |
| 2026-10-02 | Considered turning TYLI into a local web server (C++ HTTP layer on hoardor + a browser UI) and decided to stay a Qt desktop app. The web option was easier for the UI and reachable from any device, but browsers limit playback (formats like ALAC/APE/DSD, MKV with HEVC/AC3/DTS, no bit-perfect output) unless the server transcodes. hoardor stays headless, so a server remains possible later. |
| 2026-10-02 | The look is OLED black + minimal green (`features/theme.md`), on the Basic style with TYLI's own controls; Fusion is dropped. It replaces the Proposed "Fusion, follows the system's light or dark mode". |
| 2026-10-02 | A GitHub Actions workflow (`.github/workflows/windows.yml`) builds TYLI with hoardor on Windows (MSVC 2022, Qt 6.12), runs the tests, and uploads a ready-to-run zip, so the user can test on their PC without installing Visual Studio and Qt. It's a Windows build, not full CI: CI on all three OSes stays deferred (hoardor's decision log, 2026-10-01). |
