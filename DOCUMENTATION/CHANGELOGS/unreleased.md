# Unreleased

Work that is done but not yet part of a version. The newest entries come first.

### Search off the UI thread; back from a result returns to the results (2026-10-03, branch `abhinavp06/PLAYER`)

**Summary:** The user: "search feels slow… maybe run a UI test yourself", and "if I go back… I want to go back to the search results, not the homepage".
- **Measured** (hoardor `BM_SearchKeystroke`, 50k tracks): about 0.5 s per keystroke for "t", "tr", "track", and "track 1", all on the UI thread. That's an album count of 125 ms, an album page of 104 ms, the same again for artists, a track count of 60 ms, and a track page of 53 ms. No single query was the problem: the UI thread was.
- **Fix:**
  - `Library.search(text)` runs the whole search on Qt's thread pool with its own connection, fetching only what the page shows (12 per section, 24 artists) plus totals.
  - Only the latest text is answered: older searches still running are dropped, and the newest runs next, so work never piles up.
  - `SearchView` shows the results it's given (`searching "…"…` until then).
  - The typing delay went from 200 ms to 400 ms, and search needs 2 letters.
- **Back to the results:**
  - opening a result marks that page (`CategoryView.returnDepth`); back from it (arrow, mouse button, Alt+←) calls `returnRequested`, and `Main.reopenSearch()` shows the results again at once, as they were (they stay in the page while it's hidden)
  - a breadcrumb above the page, or another category, drops it
- **Mockup for review** (the other session's work, TYLI `d5c55eb`, before any QML): `player-expanded.html`.
  - The bar's cover shows ↑ to expand.
  - The expanded view has a large cover, artist and album links, and ↓ at the top right.
  - The queue opens as a right-hand panel with its own ×, while the cover shrinks.
  - The bar's cover becomes ↓ while expanded, and artist and album are links in the bar.

**Decisions**
- **Off the UI thread, not just a longer delay:** a 400 ms delay alone still froze the window for 0.5 s after every pause in typing.
- **Two letters minimum:** one letter matches most of a library, is the slowest query, and is never what you're looking for.
- **The rule "reading SQLite on the UI thread is fine because it's paged"** doesn't hold for a full-text match with counts over a big library. Search is the first read moved off the UI thread.

**Files**
- `src/library_bridge.{hpp,cpp}` (`search`, `searching`, `searchFinished`, `runSearch`)
- `qml/SearchView.qml`, `qml/Main.qml` (`returnSearch`, `reopenSearch`, 400 ms), `qml/CategoryView.qml` (`returnDepth`, `returnRequested`)
- `tests/tst_media_models.cpp`, `tests/tst_qml_smoke.cpp`
- `DOCUMENTATION/application/features/media_listing_ui.md` (§7c)

**Tests:** all 5 suites pass.
- `TestMediaModels::searchAnswersOnlyTheLatestText`: three texts typed in a row are answered once, for the last; the sections' totals and first rows; an empty text answers nothing.
- The smoke test:
  - a one-letter hint
  - an album opened from the results, then the mouse's back button: the results show again, with the same text
  - screenshots `search-too-short` and `search-after-back`

### Windows build: ship the Vulkan loader for libmpv (2026-10-03, branch `abhinavp06/PLAYER`)

**Summary:** The first Windows run with the player failed every test at startup with `0xc0000135` ("DLL not found").
- **Cause:** `libmpv-2.dll` imports `vulkan-1.dll`, the Vulkan loader (checked with `objdump -p` on the pinned DLL). GPU drivers usually install it into System32, but the runner has no GPU driver. Every other import is part of Windows.
- **Fix:** the workflow fetches LunarG's Vulkan runtime components (1.4.363.0, pinned by SHA-256), puts the x64 `vulkan-1.dll` next to `libmpv-2.dll` for the tests, and ships it in the zip with `vulkan-LICENSE.txt`. The libmpv cache key changed, so the cached folder is rebuilt with it.
- **Why shipping it is safe:** TYLI draws video through OpenGL (mpv's render API). On a machine without Vulkan drivers, the loader simply finds none, and mpv doesn't need Vulkan.

**Files:** `.github/workflows/windows.yml`

### The player in TYLI: now-playing bar, queue prompt, queue page, video page, likes, markers (2026-10-03, branch `abhinavp06/PLAYER`)

**Summary:** The approved player design (`features/player_ui.md`, mockups `player-*.html`), built on hoardor's new `player` engine.
- **Music:**
  - a track click plays the album from it; while something plays, the queue prompt asks (cancel / add to queue (just that track) / clear queue and play)
  - the album's play button works the same way
  - the playing track is green with a ▶
  - the hearts on track rows like and unlike: a white outline, filled with the accent color when liked
- **The now-playing bar** (while something is loaded):
  - cover, title (to its album, or back to the video page), and artist · album
  - previous / play-pause / next, and a seek line with times
  - like, add to playlist (disabled: the playlists feature), the queue, mute, and volume
- **The queue page:** played rows dim, the current one green; jump, remove, clear, and the time left.
- **The video page:**
  - the picture through mpv
  - controls over it that hide (with the cursor) after 3 s
  - seek, ±10 s, the audio and subtitle menus, mute and volume, and full screen (the top bar, sidebar, and bar hidden)
  - keys: space, ←/→, f, Esc, m
  - "resumed at … · from the start"
  - leaving the page pauses (P3), and the bar's title goes back to it
- **Movies and shows:**
  - a movie plays the chosen copy, with "resume at …" in the play button's tooltip
  - an episode plays with the rest of its season (P2)
  - the show's play button resumes the episode you're partway through, or plays the next unwatched one
  - episode rows show a viewed check and a progress line
- **Under the hood:**
  - `PlayerBridge` (`Player`), `QueueModel`, and `VideoItem` (`VideoView`)
  - Qt Quick on OpenGL (P4)
  - `CategoryView.play()`, and `Main.view` choosing the page

**Decisions and findings**
- **A video waits for its picture:** `pendingVideo` starts once `VideoView.ready`, so mpv never plays a video it can't draw.
- **The video page stays drawn under a cover while a video is loaded,** because mpv waits up to 0.2 s for each undrawn frame. A safeguard: a test failure I first blamed on this turned out to be Qt Test cutting a wait short after an earlier failed check.
- **`Player.revision >= 0 && …` in bindings,** not a bare `Player.revision;` statement, which can be compiled away. The bar's heart then intermittently missed a like (about 1 run in 7 under ctest).
- **Checked on the VNC display:** the picture draws on OpenGL. A 160×90 clip blown up on llvmpipe showed a grid of lines (software upscaling); a 960×540 clip is clean.

**Files**
- **New:** `src/player_bridge.{hpp,cpp}`, `src/queue_model.{hpp,cpp}`, `src/video_item.{hpp,cpp}`, `qml/PlayerBar.qml`, `qml/QueuePrompt.qml`, `qml/QueuePage.qml`, `qml/VideoPage.qml`, `qml/SeekLine.qml`, `tests/tst_player_bridge.cpp`
- **Changed:** `src/main.cpp`, `qml/Main.qml`, `qml/CategoryView.qml`, `qml/AlbumView.qml`, `qml/TrackList.qml`, `qml/MovieView.qml`, `qml/ShowView.qml`, `qml/PlayButton.qml`, `qml/Icon.qml`, `CMakeLists.txt` (Qt OpenGL, the new files), `tests/CMakeLists.txt` (`TYLI_PLAYER_OUTPUTS=null`), `tests/tst_qml_smoke.cpp`, `CLAUDE.md`
- **Docs:** `DOCUMENTATION/application/{ARCHITECTURE.md, features/player_ui.md}`

**Tests:** all 5 suites pass (two runs in a row).
- **`tst_player_bridge`** (new):
  - playing describes the track (title, artist · album, the link to its album, the cover entry); the queue model's rows, playing/played marks, and time left
  - three unplayable files make one status line
  - likes and states come back, also after a restart
- **The smoke test's `playingMusicAndVideo`**, with real tones and a clip when the ffmpeg tool exists:
  - the album plays without a prompt, and the bar follows a like
  - a track click opens the prompt, and "add to queue" adds one track
  - the queue page lists 7
  - a movie opens the video page and replaces the queue; the back button leaves it and pauses
  - screenshots
- **Also run on the VNC display with OpenGL** (`TYLI_PLAYER_OUTPUTS=noaudio`): the picture draws.

**Not built yet:** markers on movie cards, the artist page's play button, a space-bar toggle outside the video page, showing which copy plays.

**To check on Windows:** playback on the user's library (music gapless, a 4K remux, subtitles), the picture with a real GPU on OpenGL, the zip with `libmpv-2.dll`, and OI-1 (a sync on the same HDD while a movie plays).

### Windows build: libmpv (2026-10-03, branch `abhinavp06/PLAYER`)

**Summary:** hoardor's new `player` engine links libmpv, so the Windows workflow now fetches it.
- **The package:** a pinned shinchiro `mpv-dev-x86_64` build (20261003, git `3186d369f9`), checked by SHA-256 and cached.
- **The MSVC import library:** the package has only a MinGW one, so `mpv.lib` is made from the DLL's exports (`dumpbin /exports` → `.def` → `lib /def`), after the MSVC setup step.
- **Configure** passes `-DMPV_ROOT`, and the tests have `mpv\bin` on `PATH`.
- **The zip** ships `libmpv-2.dll` with `libmpv-NOTICE.txt` (where it comes from; GPL v2+, because the build bundles GPL parts; hoardor `features/player.md` §9).

**Files:** `.github/workflows/windows.yml`

**To check:** the first run on GitHub (it answers hoardor's OI-P3: the import library in MSVC).

### Player UI approved; the like heart (2026-10-03, branch `abhinavp06/PLAYER`)

**Summary:** The user approved the design and the mockups, and asked for a heart next to the add-to-playlist button: green when liked, empty when not.
- **The now-playing bar** now has three distinct buttons: ♥ like (white outline; green and filled in `Theme.accent` when liked), add to a playlist (later), and the queue. The queue used to share the playlist icon, so it has its own now.
- **The hearts on track rows toggle too,** with the same look. hoardor stores the likes (`player_items.liked_ns`).
- **The five proposed decisions (P1–P5)** were approved with the design.

**Files**
- `DOCUMENTATION/mockups/player-music.html`, `DOCUMENTATION/mockups/player-queue.html`
- `DOCUMENTATION/application/features/player_ui.md` (status: approved; the bar, the rows, the bridge, the models)

### Player UI designed, with mockups (draft, awaiting approval) (2026-10-03, branch `abhinavp06/PLAYER`)

**Summary:** TYLI's side of the player (hoardor `features/player.md`, libmpv in a new `player` engine). `features/player_ui.md` designs:
- **The queue prompt** on a track click, only while something plays (cancel / add to queue / clear queue and play).
- **The now-playing bar.**
- **The queue page.**
- **The video page:** overlay controls, audio and subtitle menus, full screen, keys, resume.
- **Viewed and progress markers.**
- **The bridge:** `PlayerBridge` as the `Player` singleton, `QueueModel`, a `VideoItem` on mpv's OpenGL render API, and progress roles in the models.
- **Five decisions** for the user (P1–P5): a video replaces the queue without a prompt; an episode queues the rest of its season; leaving the video page pauses; Qt Quick draws with OpenGL; the queue is a page.

**Mockups** (new, marked "proposal · player"): `player-music.html` (album page with the prompt, the playing track, and the now-playing bar), `player-queue.html`, `player-video.html`, `player-markers.html`. `index.html` lists them, and the media-library pages are now under "built (v0.2.0)".

**Files**
- `DOCUMENTATION/application/features/player_ui.md` (new)
- `DOCUMENTATION/mockups/{player-music,player-queue,player-video,player-markers}.html` (new), `DOCUMENTATION/mockups/index.html`
- `DOCUMENTATION/application/ARCHITECTURE.md`

