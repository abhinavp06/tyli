# Unreleased

Work that is done but not yet part of a version. The newest entries come first.

### Detail pages: no empty band under the breadcrumbs (2026-10-03, branch `abhinavp06/PLAYER`)

**Summary:** On an album, movie, show, or season page, `BrowseHeader` drew its title row and hairline with nothing in them. That left about 70 px of empty space between the breadcrumbs and the page's own hero. The mockups have none.
- **Fixed:** the row and its line show only when the header has a title, a label, a subtitle, or the sort (`titled`). Grids and the artist, genre, and director pages are unchanged.

**Tests:** 5 suites pass. Screenshots reviewed: album, movie, show, season, artist.

**Files:** `qml/BrowseHeader.qml`

### Shows: seasons as album cards, a season page laid out like an album (2026-10-03, branch `abhinavp06/PLAYER`)

**Summary:** Part 2 of "music's album structure for movies and shows" (mockups `show-seasons.html` and `season.html`, approved).

**What changed:**
- **`ShowView`:**
  - The hero's poster is as tall as an album cover (147 px wide).
  - The show's quality picker and play (the next episode) stay.
  - The season tabs and episode list are replaced by a "seasons" `CardFlow` of `CoverCard`s, specials and unknown seasons last. Each card has the show's poster (hoardor keeps no season posters yet), "N episodes · year" ("· watched" when all are), and a watched line.
- **`SeasonView` (new, route `season`, crumbs shows / Dark / Season 2):**
  - the album layout: label "season", title, its show as a link back, year, genres, directors, "N episodes · runtime · M watched", and the drive
  - a quality picker with that season's qualities and episode counts, sharing the show's remembered choice
  - play (the episode you're in, else the next unwatched one of the season)
  - the episode list moved from `ShowView` (markers, centered rows, right-aligned quality)
- **`ShowState` (new):** one show's state shared by both pages:
  - `Library.showCopies`, refreshed on `contentChanged`
  - seasons in order, season titles, qualities (whole show or one season)
  - the chosen quality, `pick`, what you watched, `seasonProgress`, `playFrom`, `playNext`
- **`CoverCard.progress`:** a 2 px watched line under the art.
- **`Library.showCopies`** also returns `coverEntry`, the show's card cover, so a show or season page opened without one shows it.

**Tests:** 5 suites pass.
- `tst_qml_smoke::showsAndAShow`: season cards (titles, "3 episodes"); opening season 2 (2 episodes); the show link back; season 1 (3 episodes)
- `tst_media_models`: `showCopies` has a cover

Screenshots: `show`, `season`.

**Files:**
- `qml/{ShowView.qml, SeasonView.qml (new), ShowState.qml (new), CoverCard.qml, CategoryView.qml}`
- `src/library_bridge.cpp`, `CMakeLists.txt`, `tests/{tst_qml_smoke.cpp, tst_media_models.cpp}`
- `DOCUMENTATION/application/{features/media_listing_ui.md, ARCHITECTURE.md}`

**Follow-up:** season posters (`season02-poster.jpg`, Plex's season poster) in hoardor.

### Movies and shows grids: "by director" and "by genre" sections (2026-10-03, branch `abhinavp06/PLAYER`)

**Summary:** Part 1 of "music's album structure for movies and shows" (mockups `movies-by.html` and `shows-by.html`, approved). Uses hoardor `f02c809` (`Condition::none`).

**What changed:**
- **The sort bar:**
  - movies grid: "by director" and "by genre"
  - shows grid: "by genre" (an episode's directors vary, so shows have no "by director")
  - genre and year pages keep added / a–z / year
- **`VideoGridView`:**
  - In a section sort, the grid gives way to a `ListView` of sections: the name (a link to the director's or genre's page), its count, a hairline, then its cards (`CardFlow`, a–z, the first 120).
  - A footer section "no director" / "no genre" holds the rest. Its filter is `{director: null}`, which `fields::videoFilter` now turns into `Condition::none`.
  - Card subtitles and opening are shared with the grid (`subtitleOf`, `openRow`).
- **Detail:** a section's count sat higher than its bigger name (a top-aligned `Row`). It's on the name's baseline now, in music's "by artist" too.

**Tests:** `tst_qml_smoke`:
- `moviesAMovieAndADirector`: by director, 2 sections and no "none" section; by genre, 2 sections and "no genre" for Heat
- `showsAndAShow`: by genre

Screenshots: `movies-by-director`, `movies-by-genre`, `shows-by-genre`.

**Files:** `qml/VideoGridView.qml`, `qml/AlbumBrowser.qml`, `src/media_fields.cpp`, `tests/tst_qml_smoke.cpp`, `DOCUMENTATION/application/features/media_listing_ui.md` (§7g)

### Subtitles on by default (with hoardor 4f0f678) (2026-10-03, branch `abhinavp06/PLAYER`)

**Summary:**
- **New default:** with hoardor `4f0f678`, a video starts with subtitles on: a track in the preferred languages, else the first full one, embedded or a `.srt` next to the file.
- **TYLI's change:** the video page's subtitle menu marks forced tracks "forced", unless their own title already says so.
- **Not built yet:** a subtitles switch and preferred languages in Settings.

**Files:** `src/player_bridge.cpp`

### Media lists fill the width, edge to edge (2026-10-03, branch `abhinavp06/PLAYER`)

**Summary:** The user: "a lot of negative space on the right while listing any media", with the sort options at the far right and the cards ending well before them. Fixed as its own commit.

**What was wrong:**
- **`CoverGrid` lost a column.** It computed `columns` from the width, but each cell held its card plus the gap after it. `columns` cells came out one gap wider than the view, so `GridView` fitted one column fewer and left an empty strip about a card wide. At 1280×800: 4 albums in a row and about 200 px empty, where 5 fit.
- **Fixed-width cards:** the artist page, search results, the director and actor page, and the album browser's by-artist sections used 172 px cards (posters in search 150 px) in a `Flow`, leaving a ragged gap.
- **Fixed-width search track rows:** they used 360/260/260 px columns.
- **Show page episode list:**
  - Its quality column was left-aligned in 120 px, so the row looked unfinished on the right.
  - Its header had only "#" and "name".
  - Each episode's number sat about 10 px above its name: the check cell is full height, so the centered name dropped while the other cells stayed top-aligned.

**What changed:**
- **`CoverGrid`:** cells split the width evenly (`width / columns`), and each card shifts within its cell, so the last column ends exactly at the right edge, under the sort.
- **`CardFlow.qml` (new):** a `Flow` that sizes cards like `CoverGrid` (at least 172 px, stretched to fill). Used by `ArtistView`, `SearchView` (albums, movies, shows), `PersonView`, and `AlbumBrowser`'s by-artist sections. Search's posters now match the grid's.
- **Search track rows:** 40/30/30 % of the width.
- **Episode rows:**
  - every cell centered
  - quality right-aligned, like the album page's bit rate
  - header labels "length" and "quality" over their columns, sharing one `nameWidth`

**Tests:** 5 suites pass. `tst_qml_smoke::musicAlbumsInEverySort` checks that the grid's last computed column is on the first row and its card ends within 1 px of the grid's right edge. It fails on the old layout, where the fifth card wrapped. Screenshots reviewed: music grid, search, artist, director, show, movies, queue.

**Files:** `qml/{CoverGrid.qml, CardFlow.qml (new), ArtistView.qml, SearchView.qml, PersonView.qml, AlbumBrowser.qml, ShowView.qml}`, `CMakeLists.txt`, `tests/tst_qml_smoke.cpp`

### The sidebar (and the queue button) leave search results (2026-10-03, branch `abhinavp06/PLAYER`)

**Summary:** The user: with search results showing, clicking a section in the sidebar showed the hand but went nowhere. The results outrank every page (`view` puts search first), and the sidebar's click reset every page flag except the search.
- **Fixed:** a section click clears the search (and its "back to results" memory), then goes to the category.
- **Same gap, found while checking every page switch:**
  - The queue button set `queueOpen`, which search results and Settings both outrank. It now opens the queue over them, and closes it only when the queue is the page shown.
  - Playing a video now also clears a search and leaves Settings.
- **The logo:** `tyli_` already cleared the search (`openHome`). Its test passes with a real click while the field has focus. It now also forgets the "back to results" memory.

**Tests:** `tst_qml_smoke::theSidebarAndTheLogoLeaveSearch`, with real clicks:
- a section from results
- the logo from results
- a section from results over Settings
- the queue button over results and over Settings, and closing it again

The sidebar items have object names (`sidebarItem-<name>`).

**Files:** `qml/Main.qml`, `qml/Sidebar.qml`, `tests/tst_qml_smoke.cpp`

### The search field's × no longer covers its outline (2026-10-03, branch `abhinavp06/PLAYER`)

**Summary:** The user noticed that hovering the × drew its grey background over the right edge of the field's green outline. The button sat flush right and was sized 2 px short of the field's height, so it covered the 1 px outline on the right.
- **Fixed:** it's inset by 3 px, with 6 px less height, so a 2 px black gap stays between its hover background and the outline on every side. The text's right padding follows.
- **Checked:** no other icon button sits inside a bordered box. The others are in rows or headers.

**Tests:** `tst_qml_smoke::searchFromSettingsAndClear` hovers the × and checks two things:
- the button stays at least 2 px inside the outline on each side
- the outline's right column is one color from top to bottom in the grabbed window

Both checks fail on the old layout. Screenshot: `search-clear-hovered`.

**Files:** `qml/TopBar.qml`, `tests/tst_qml_smoke.cpp`

### CI: test videos made with ffmpeg's own encoder (2026-10-03, branch `abhinavp06/PLAYER`)

**Summary:** On Windows, `tst_media_models::videoPostersFromPlexThenFrames` failed with "Unknown encoder 'libx264'". The runner's ffmpeg tool is the LGPL build, and x264 is GPL.
- **Found on the way:** the smoke test's real media had the same problem, quietly. Its Heat clip was never made on Windows, so the player's screens were skipped there.
- **Fixed:** both tests encode with `mpeg4`, ffmpeg's built-in encoder, which is in every build.
- **No more quiet skips:** the smoke test now fails when the tool is installed but can't make its media.
- **Expect** the player's screens to run on the Windows runner from now on.

**Files:** `tests/tst_media_models.cpp`, `tests/tst_qml_smoke.cpp`

### Search: a clear button, and search from any page (2026-10-03, branch `abhinavp06/PLAYER`)

**Summary:** Two requests from the user: "add an x button in search to clear things", and "search should work from any screen" (it didn't from Settings: Settings outranked the results in `Main.qml`'s `view`).

**What changed:**
- **`TopBar.qml`:** a `TyIconButton` (×, `clearSearch`) inside the field while it has text. It clears the field and keeps its focus. The text no longer runs under it.
- **`Main.qml`:**
  - `view` puts search before Settings, and `SettingsView` shows only when `view === "settings"`.
  - Back clears the results first, then leaves Settings.
  - `openSettings()` clears a search.
  - The gear shows Settings over results and closes them only when they're showing. It's lit by `view`.
  - Opening a result leaves Settings.
  - An emptied field shows the page at once, instead of after the 400 ms typing delay.

**Tests:** 5 suites pass. `tst_qml_smoke::searchFromSettingsAndClear`:
- results over Settings
- the × clears at once, back on the same Settings page, with the field focused
- back from results over Settings, then back out of Settings

Screenshot: `search-over-settings`.

**Files:** `qml/TopBar.qml`, `qml/Main.qml`, `tests/tst_qml_smoke.cpp`, `DOCUMENTATION/application/features/media_listing_ui.md`

### Posters for every movie and show: Plex's, else a frame (2026-10-03, branch `abhinavp06/PLAYER`)

**Summary:** Most movies and shows had no poster on the user's Windows test, because Plex keeps its art in its own data folder. The user chose Plex's posters, with a frame from the video as the fallback. Together with hoardor `3f59b51` (`PlexPosters`, `grab_frame`, `Group::first_entry`). `features/media_listing_ui.md` §7f.

**What changed:**
- **`ThumbnailCache`:** after the poster image and the poster inside the video come Plex's poster, then a frame from the group's first video.
- **When the fallbacks run:**
  - Frames only in passes after tag reading (`rebuild(bool frames)`; the 15-s passes during reading use `false`), since they read the drive.
  - Plex's database is read once per pass, only when something needs it.
  - A failed frame, or (during reading) a Plex miss, isn't retried in the same session (`Options` and `Result` carry the sets).
- **`fields::videoCover(group)`:** one rule for the entry a video card's thumbnail is named after. Used by `GroupModel`, search, and `PlayerBridge`: the bar now shows the video's movie or show poster (any copy's, Plex's, or a frame).
- **`TYLI_PLEX_DIR`** points to another Plex data folder.
- **`tools/plex_probe.ps1`:** a read-only check of Plex's posters on Windows, shipped in the zip.
- **The workflow** ships `swscale-*.dll` (frames) and the probe.
- The `Outputs` initializers name `log_file` (a warning).

**Tests:** 5 suites pass. `tst_media_models::videoPostersFromPlexThenFrames`:
- a fake Plex folder, a real video, and junk
- Plex's poster in a pass without frames, matched across a drive letter
- the frame in a pass with frames
- the junk's failed frame not retried
- 2 of 3 cards with thumbnails

The smoke test's movie grid shows a frame as Heat's poster.

**Files:**
- `src/{thumbnail_cache.hpp, thumbnail_cache.cpp, media_fields.hpp, group_model.cpp, library_bridge.cpp, player_bridge.cpp}`
- `tests/tst_media_models.cpp`, `tools/plex_probe.ps1` (new), `.github/workflows/windows.yml`, `CLAUDE.md`
- `DOCUMENTATION/application/{features/media_listing_ui.md, ARCHITECTURE.md}`

**Known limitations:**
- Verified against a fake Plex, not the user's.
- A frame poster stays after Plex gets a real one, until its thumbnail is cleared.
- The first run grabs a frame per movie and show without art. That takes about a second each on a spun-up HDD, in the background after tag reading.

### Video grain checked; TYLI_MPV_LOG; the "Resume" idea recorded (2026-10-03, branch `abhinavp06/PLAYER`)

**Summary:**
- **"Video playback is a little grainy":** measured on the VM, with a 10-bit gradient clip through `VideoView` on OpenGL compared with ffmpeg's clean scaling (hoardor `features/player.md` §10b).
  - The video surface has the item's full device pixels.
  - It's 8 bits per channel, so mpv dithers 10-bit video, about one step of noise. That's the grain, and it's at the reference conversion's level.
  - TYLI's path adds nothing.
- **`TYLI_MPV_LOG=1`** makes mpv write `mpv.log` next to `library.db`, to see what the user's Windows libmpv chooses (it's newer than the VM's 0.37).
- **Recorded, not built:** the user's idea of separate music and video players, or a deliberate merge, with a "Resume" section in the sidebar. OI-U3 in `features/player_ui.md`.
- **Seen while measuring (open):** in the smoke test on this VM's software OpenGL, a video sought and paused right after starting sometimes showed no picture. mpv logged "mpv_render_context_render() not being called". Each frame mpv announced did lead to a redraw, so this looks like seek/pause timing on llvmpipe; to watch for on a real GPU.

**Files:** `src/player_bridge.cpp`, `DOCUMENTATION/application/features/player_ui.md` (§5b, OI-U3)

### The expanded player; artist and album links in the bar; a docs audit (2026-10-03, branch `abhinavp06/PLAYER`)

**Summary:** Built from the approved mockup `player-expanded.html` (the user: "looks good.. build and push").
- **The bar's cover** shows ↑ on hover. A click expands the player over everything above the bar: a large cover, the title, artist · album (links) · year · quality, and ↓ at the top right.
- **While expanded,** the bar's queue button opens the queue as a right-hand panel, with its own × and "clear". The cover shrinks to stay in view.
- **Collapsing:** the bar's cover (now ↓), ↓ at the top right (also with the queue open), Esc, or back. Links collapse the player and open their page.
- **The bar's subtitle** is now artist and album as links (music; video stays plain).

**Decisions**
- **The queue list is shared:** `QueueList`, used by the queue page and the panel (compact, without the artist · album column).
- **The expanded view takes the keyboard while open,** so Esc works. A window shortcut lost the key to a field that had focus (it failed in the smoke test).
- **`openLink()` decides by the link's page,** not by comparing with `Player.link`. A map property read from C++ is a new JavaScript object each time, so `===` never matched.

**Docs audit** (the user asked whether every document keeps up):
- **hoardor:** `CODE_TREE.md` lists every file, checked by script; `DATABASE.md`'s migration history matches the code (file 1–3, audio 1–2, video 1–3, player 1). Fixed: `engines/master.md` gains §1b (`file_resolver`); ARCHITECTURE calls the player built, with a decision row.
- **TYLI:**
  - ARCHITECTURE's threads table gains playback, video frames, and search, and the layers line now says playback; two decision rows
  - `features/player_ui.md` §1.6
  - `CLAUDE.md`: the Windows zip's libmpv and Vulkan loader, and building with libmpv
  - TYLI has no code tree of its own: `CLAUDE.md`'s layout lists the sources

**Files**
- **New:** `qml/ExpandedPlayer.qml`, `qml/QueueList.qml`
- **Changed:**
  - QML: `qml/PlayerBar.qml`, `qml/QueuePage.qml`, `qml/Main.qml`, `qml/Icon.qml` (`chevronUp`)
  - C++: `src/player_bridge.{hpp,cpp}` (`artist`, `album`, `artistLink`, `detail`)
  - build and tests: `CMakeLists.txt`, `tests/tst_qml_smoke.cpp`
  - docs: `CLAUDE.md`, `DOCUMENTATION/application/{ARCHITECTURE.md, features/player_ui.md}`

**Tests:** all 5 suites pass, in two runs in a row. The smoke test's `playingMusicAndVideo` now also covers:
- the bar's cover expands the player; the bridge names the artist and album
- the queue button opens the panel, not the page, and its × closes it
- the artist link opens the artist and collapses; Esc collapses
- screenshots `player-expanded` and `player-expanded-queue`

**Limitation:** the large cover is the 300 px thumbnail, scaled up, so real covers will look soft. A larger cached size is a follow-up.

### Mockup fix: the expanded player showed a black screen (2026-10-03, branch `abhinavp06/PLAYER`)

**Summary:** The user, reviewing `player-expanded.html`: "expanding just shows a black screen".
- **Cause:** the class `expanded` was both the window's state class (`.window.expanded`) and the expanded view's own class, whose rule hides it (`display: none`). Expanding gave the window that class, so the rule hid the whole window.
- **Fix:** the view's class is now `.xview`. With the queue open, the explanatory note no longer runs under the queue panel.
- **Checked** with a headless Chromium (Playwright, in Claude's scratch folder, not the repo): screenshots of the bar, expanded, expanded + queue, and collapsed again, with no script errors.

**Files:** `DOCUMENTATION/mockups/player-expanded.html`

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

