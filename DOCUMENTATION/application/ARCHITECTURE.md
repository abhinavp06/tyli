# TYLI Architecture

This is a living document. It records TYLI's system-wide decisions and why they were made. The engine's architecture is in `../hoardor/DOCUMENTATION/application/ARCHITECTURE.md`, and §1 there ("hoardor vs TYLI") is binding for both repositories.

Status markers: **Decided** means agreed with the user. **Proposed** means it still needs the user's approval or a measurement.

## 1. Layers (Decided)

```
QML views (qml/)                  what the user sees; no business logic
      │  properties, invokables, signals, list models
C++ bridge (src/)                 QObjects and QAbstractListModels: converts hoardor types to Qt types,
      │                           and moves hoardor's background callbacks onto the UI thread
hoardor (../hoardor, add_subdirectory)   all real work: SQLite, scanning, sync, metadata, playback (libmpv)
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
| Playback (since 2026-10-03) | `hoardor::player::Player` (hoardor's own thread; libmpv runs its own) | The same: `PlayerBridge` forwards status, position, queue, and error callbacks with queued calls. Commands never block the UI thread |
| Video frames | Qt's render thread (`VideoItem`, mpv's OpenGL render API) | mpv asks for a redraw; the item draws on the next frame |
| Search (since 2026-10-03) | A Qt thread-pool task with its own hoardor connection (`Library.search`) | `searchFinished(text, results)`; only the latest text is answered. A full-text search with counts over a big library took about 0.5 s, too long for "reading on the UI thread is fine" |

Why: the UI must never freeze. A sleeping HDD takes 5–10 s to spin up, and anything that touches it must not run on the UI thread.

## 3. Data location (Proposed)

- **The library database** is `QStandardPaths::AppDataLocation/library.db`: on Windows `%APPDATA%\TYLI\library.db`, on Linux `~/.local/share/TYLI/library.db`. That's always internal storage, never a media drive (hoardor ARCHITECTURE §3).
- **UI preferences** (sort choices, remembered qualities, track columns, category views, scroll speed) are `preferences.ini` in the same folder (since 2026-10-03). Before, they went through `QSettings()`, which on Windows saves nothing without an organization name (found through the CI failure). Setting an organization name would have moved the data folder to `%APPDATA%\TYLI\TYLI`.
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
| 2026-10-02 | Order of work after `v0.1.0` (File Sync UI + theme, together with hoardor `v0.1.0`): media listing (a basic view per category from file and folder names), then a first player draft (audio and video), then metadata (tags, cover art, chapters; later Skip Intro). It replaces putting metadata and the album view on `abhinavp06/FILE_SCANNER_INIT`. hoardor's `engines/file.md` §4.0 holds the order. |
| 2026-10-02 | **Media library v1 (the user's review of the media-listing mockups):** names come from metadata (hoardor reads it with ffmpeg on this branch, before the player). TYLI builds every layout from hoardor's generic filter/order/group queries, groups copies of an album, movie, or episode into a quality picker (highest by default), and owns cover and poster thumbnails (`AppData/thumbnails/`). Which grid a category opens is a TYLI preference, not a hoardor field. `features/media_listing_ui.md`. |
| 2026-10-02 | **UI rules from the same review:** actions (play, quality, …) sit at the top right of their page. One shared sort bar and header for every grid. Content shows names exactly as tagged, while the UI's own labels stay lowercase. Artist, year, genre, director, and drive are links. The queue prompt (with the player) is the one deliberate popup. |
| 2026-10-02 | **Copies and playback:** the player will queue specific files (entry ids), never "an album", so copies found later (another quality) never change what's playing. A quality picked on an album, movie, or show page is remembered per item; "best by default" applies only without a choice. Grids refresh in place while tags are read (every 2.5 s), with no resets. |
| 2026-10-03 | **Back and forward like a browser** (the user's Windows test): `CategoryView` keeps the pages you came from alive in a `StackView`, so going back restores the scroll position. The mouse's side buttons, Alt+←/→, and the sidebar's current category navigate. History doesn't span categories. `features/media_listing_ui.md` §7d. |
| 2026-10-03 | **Sizes:** one scale (`Theme.scale` 1.15, `Theme.px()`) for every length. The play button and the quality picker stay at 40 px. Screens of different sizes rely on Windows' per-monitor display scaling through Qt, with no TYLI setting. The mouse wheel scrolls 1.2× as far, through one app-wide filter (`WheelSpeed`). `features/theme.md` §5b. |
| 2026-10-03 | **The wheel speed became a setting** ("scroll speed", default 200%) after 1.2× still felt slow. **`tyli_` opens a home page** (blank until it's designed after the player and playlists; recommendations will live there). Every clickable control shows the hand cursor. |
| 2026-10-03 | **UI preferences live in `preferences.ini` next to `library.db`**, not in `QSettings()`, which saved nothing on Windows (no organization name). Values come back as the fallback's type. §3. |
| 2026-10-03 | **The player** (the user): libmpv in hoardor's new `player` engine. First draft: the music queue with the queue prompt, a now-playing bar, a video page, resume and viewed markers. *Proposed, pending approval:* `features/player_ui.md`, including Qt Quick on OpenGL everywhere (mpv's render API) and a second bridge object (`Player`), since it holds state. |
| 2026-10-03 | **The player is built** (`features/player_ui.md` §5a). Decisions P1–P5, approved with the design: a video replaces the queue without a prompt; an episode queues the rest of its season; leaving the video page pauses; Qt Quick draws with OpenGL everywhere (`main.cpp`); the queue is a page. A second bridge object, `Player`, holds hoardor's `Player`. |
| 2026-10-03 | **Search runs off the UI thread** (`Library.search`): at 50k tracks a keystroke took about 0.5 s of queries. The first library read moved off the UI thread (§2). Back from a search result returns to the results. |
| 2026-10-03 | **Playlists and Liked Songs** (the user's design review): music only, manual playlists, with the user's own order and pinned ones first; Liked Songs fixed on top; a speaker for "playing from here". Built on hoardor's `player::Library`; the calls live on the `Player` bridge (it holds that library). A top-level playlist page beside the queue page. `features/playlists_ui.md`. |
| 2026-10-03 | **Music's album structure for movies and shows** (the user; mockups approved): grids by director and by genre, a show's seasons as album cards with a season page laid out like an album, and the movie page in the album layout (copies as rows, more by the director and in each genre). The folder rule is deferred by the user. `features/media_listing_ui.md` §7g. |
| 2026-10-03 | **Posters from Plex, else a frame** (the user): for a movie or show without art, the thumbnail pass takes Plex's poster (hoardor reads Plex's data read-only), else a frame from the video (only after tag reading, since it reads the drive). `features/media_listing_ui.md` §7f, hoardor `features/posters.md`. |
| 2026-10-03 | **The expanded player** (mockup `player-expanded.html`, approved): the bar's cover expands it over everything above the bar, the queue opens there as a right-hand panel, and artist and album are links in the bar. `features/player_ui.md` §1.6. |

