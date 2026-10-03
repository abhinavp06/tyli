# TYLI

TYLI is a fully offline, native alternative to Plex for music, movies, TV shows, and text (books, blogs, notes). It's the **application**: windows, QML views, player controls, visualizer. The engine (scanning, storage, metadata, decoding) is **hoardor**, a separate C++ library that TYLI builds from the sibling folder `../hoardor`.

The architecture, the decisions behind it, and their rationale are in the file below. Treat it as binding. Change it only through an explicit decision with the user, and record that decision in it.

@DOCUMENTATION/application/ARCHITECTURE.md

## Workspace

TYLI lives in `/root/tyli-workspace/` next to hoardor. The workspace root has its own `CLAUDE.md` (not version-controlled), which imports this file and hoardor's and holds the cross-repo rules. Every path in this file is relative to the TYLI repository root. hoardor's own rules are in `../hoardor/CLAUDE.md`.

## Common goal

A snappy, low-memory, fully offline home for the user's whole media and writing library. TYLI makes it pleasant to use; hoardor does the work. The UI must never freeze, and must never wake a sleeping drive just to show something.

## Session start: read in this order

1. **`CLAUDE.md` + `DOCUMENTATION/application/ARCHITECTURE.md`** load automatically: *how* TYLI is built.
2. **`DOCUMENTATION/CHANGELOGS/`**: read `unreleased.md` in full, and the newest `v*.md` (see the index in `README.md`): *where we are*.
3. **`git status` and `git log --oneline -15`**: the current branch and uncommitted work.
4. **`DOCUMENTATION/application/features/<feature>.md`**: *what we're building right now*. If the feature uses a hoardor API, also read that hoardor feature doc (`../hoardor/DOCUMENTATION/application/features/`).
5. **`src/`**: the C++ bridge between QML and hoardor (QObjects, list models).
6. **`qml/`**: the views.
7. **`tests/`**: what is already covered.

## Non-negotiables

- **The UI thread never blocks.** Anything that can touch a media drive (adding or relocating a folder, syncing, opening a file) runs off the UI thread. Results come back through queued signals. Reading the library from SQLite on the UI thread is fine, because it never touches a drive and is paged.
- **Memory stays flat.** Lists are `QAbstractListModel`s that fetch pages from hoardor as you scroll (`canFetchMore` / `fetchMore`). A view never loads a whole library.
- **TYLI is a thin layer.** Business logic belongs in hoardor. If a view needs data hoardor can't provide yet, the change goes into hoardor first (a hoardor feature doc), not into a TYLI workaround.
- **Cross-platform:** Windows is the user's main OS. Linux and macOS must also work. Development happens on a Linux VM. Qt hides most platform differences. Anything that doesn't go through Qt belongs in hoardor, behind its platform interfaces.
- **Customizable by design:** every hoardor setting is editable in the UI. UI-only preferences get the same "configuration, not constants" treatment.
- **Keep it simple. No bloat.** One bridge object plus list models, not a class per screen. Add a class only when something holds state.
- **Stack:** C++23, Qt 6.12 (Qt Quick, Qt Quick Controls), CMake, Qt Test. hoardor via `add_subdirectory(../hoardor)`. New dependencies are discussed with the user first.

## Repository layout

```
CLAUDE.md                     # this file
CMakeLists.txt                # root build: finds Qt, adds ../hoardor, defines tyli_ui (the QML module) and the tyli app
src/                          # C++: main.cpp, app_setup (style + fonts + wheel), LibraryBridge (the QML singleton `Library`), PlayerBridge (`Player`), GroupModel/ItemModel, QueueModel, VideoItem (`VideoView`, mpv frames), copy_grouper, thumbnail_cache, media_fields
qml/                          # QML views (one file per screen or reusable component); Theme.qml, Icon.qml, controls/ (restyled controls), fonts/
tests/                        # Qt Test suites (bridge, models, QML smoke test); screenshots land in build/screenshots/
tools/                        # developer scripts (vnc_session.sh: the UI over VNC; mockup_server.sh: the HTML mockups; plex_probe.ps1: a read-only check of Plex's posters on Windows)
DOCUMENTATION/
  CHANGELOGS/                 # one file per version + unreleased.md (same rules as hoardor)
  application/
    ARCHITECTURE.md           # TYLI's architecture and decision log
    features/<feature>.md     # plan per feature: screens, bridge API, edge cases, tests, status
  mockups/                    # HTML mockups of the UI (theme.css + one page per screen), reviewed before QML is written
```

## Build and test

```bash
# Linux VM (Qt in /opt/Qt, installed with aqtinstall; ffmpeg and libmpv dev packages for hoardor: see ../hoardor/CLAUDE.md)
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH=/opt/Qt/6.12.0/gcc_64
cmake --build build -j4     # a number: plain -j starts unlimited jobs and ran this 8 GB VM out of memory
ctest --test-dir build --output-on-failure
QT_QPA_PLATFORM=offscreen ./build/tyli     # headless run (tests, screenshots)
```

**Seeing the UI from the VM:** `tools/vnc_session.sh` runs TYLI on a virtual X display (Xvfb) and shares it with `x11vnc` **on 127.0.0.1 only**. The user connects through SSH: `ssh -N -L 5900:127.0.0.1:5900 <user>@<vm>`, then points a VNC viewer at `localhost:5900`.
- **Live view while developing:** `tools/vnc_session.sh --watch` keeps the display and VNC up and restarts TYLI whenever `build/tyli` changes, so the user's viewer shows every rebuild within about 2 s. Run it detached so it outlives the agent session: `setsid nohup tools/vnc_session.sh --watch > ~/.vnc/tyli_session.log 2>&1 < /dev/null &`.
- **From Windows:** `ssh -N -L 5900:127.0.0.1:5900 root@<vm>` in a second terminal, then TigerVNC Viewer (or RealVNC Viewer) at `localhost:5900`. On macOS use local port 5901 (`vnc://localhost:5901`), because Screen Sharing refuses `localhost:5900`.
- **HTML mockups:** UI ideas are mocked up first in `DOCUMENTATION/mockups/` (plain HTML sharing `theme.css`, committed). `tools/mockup_server.sh` serves them on `127.0.0.1:8000`, and every page reloads itself when its file or `theme.css` changes, so the user watches edits live. The user adds `-L 8000:127.0.0.1:8000` to their SSH tunnel and opens `http://localhost:8000`. Run it detached: `setsid nohup tools/mockup_server.sh > ~/.vnc/mockup_server.log 2>&1 < /dev/null &`.
- **Never use Qt's `-platform vnc`:** it ignores the address option and listens on every interface, and this VM has no firewall.
- **The VM can't see the user's external drives.** It's only for testing with sample folders. Real-drive testing needs the Windows build.

**Windows:**
- **Without installing anything:** every push runs `.github/workflows/windows.yml` on GitHub (Windows, MSVC 2022, Qt 6.12, hoardor from its public repo, same branch name or `master`). It builds, runs the tests, and uploads the artifact `tyli-windows`: `tyli.exe` with Qt's libraries, ffmpeg's DLLs (a pinned BtbN LGPL build, checked by SHA-256) with its license, `libmpv-2.dll` (a pinned shinchiro build, GPL; `libmpv-NOTICE.txt`) with the `vulkan-1.dll` it imports (LunarG's runtime, pinned; runners and some PCs have no GPU driver to provide it), and the MSVC runtime. Download it from the run's page (repo → Actions → the run → Artifacts), unzip, run `tyli.exe`. It's unsigned, so SmartScreen asks once (More info → Run anyway).
- **Requirements (building on Windows yourself):** Visual Studio 2022 (the "Desktop development with C++" workload), Qt 6.12 for `msvc2022_64` (Qt Online Installer or aqtinstall), an ffmpeg shared LGPL build (add `-DFFMPEG_ROOT=C:/ffmpeg` to the configure line, and its `bin` to `PATH`), and a libmpv dev build with an MSVC `mpv.lib` (`-DMPV_ROOT=C:/mpv`; the workflow shows how to make the `.lib`, and that `vulkan-1.dll` goes next to `libmpv-2.dll`).
- **Layout:** keep the same workspace layout, with `hoardor` next to `tyli`.
- **Build:**
  ```bat
  cmake -S . -B build -DCMAKE_PREFIX_PATH=C:/Qt/6.12.0/msvc2022_64
  cmake --build build --config Debug
  ```
- **Run:** with Qt's `bin` folder on `PATH`, or after `windeployqt build\Debug\tyli.exe`.

## Code conventions

- **C++ files:** `snake_case.cpp` / `.hpp`, `#pragma once`, namespace `tyli`.
- **Names follow Qt style,** because almost everything is exposed to QML: classes `PascalCase`, functions, properties, and signals `camelCase` (e.g. `requestSync`, `syncProgress`). This differs from hoardor on purpose. Each side follows its own ecosystem.
- **QML files:** `PascalCase.qml`, one component per file. Keep logic out of QML beyond view state; call the bridge instead.
- **hoardor types never appear in QML.** The bridge converts them to Qt types (`QString`, `QVariantMap`, models).
- **Paths:** hoardor speaks UTF-8 `std::string`. Convert with `QString::fromStdString` / `toStdString`. For folders from dialogs, use `QUrl::toLocalFile()`.
- Keep comments for the *why*, not the *what*.

## Workflow

1. **Design before code.** For every feature, write `DOCUMENTATION/application/features/<feature>.md` (screens with wireframes, bridge API, edge cases, test plan). Get the user's approval before implementing.
2. **Feature branches** `abhinavp06/<topic>`, always based on `master`. One branch is one shippable feature and one PR. Branches that pair with a hoardor feature use the same name in both repos (e.g. `abhinavp06/FILE_SCANNER_INIT`).
3. **Tests are not optional:** Qt Test for the bridge and models, plus a QML smoke test that loads every screen offscreen.
4. **Keep the docs in sync with the code, continuously.**
5. **Commit or open a PR only when the user asks.**
6. **The user is new to C++ and Qt.** Explain non-obvious Qt, QML, and CMake decisions briefly.

### Definition of done (for every feature, and before every PR)

- [ ] The build passes, and `ctest` passes on Linux. The app starts offscreen without QML warnings.
- [ ] New or changed behavior has tests.
- [ ] `DOCUMENTATION/CHANGELOGS/unreleased.md` has a complete entry.
- [ ] `DOCUMENTATION/application/features/<feature>.md` matches what was built, and its status is updated.
- [ ] `DOCUMENTATION/application/ARCHITECTURE.md` reflects any system-wide decision, with a decision-log row.
- [ ] If hoardor changed too, its own definition of done is met in its repo.

## Living documentation

Same rules as hoardor: the changelog is critical and is never rewritten. The architecture and feature docs are updated as the work happens, not at the end. Each changelog entry covers what changed and why, the decisions and rejected alternatives, every file touched, the tests and edge cases, and the known limitations and follow-ups.
