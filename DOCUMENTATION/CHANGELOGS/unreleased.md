# Unreleased

Work that is done but not yet part of a version. The newest entries come first.

### Track columns toggle from their name (2026-10-03, branch `abhinavp06/MEDIA_LISTING`)

**Summary:** The user, in Settings → General → track columns: "I don't even get the green x on clicking".
- **Cause:** each column's name was a separate `Text` next to a bare `[ ]` box, so a click on the name did nothing. Only the three-character box was clickable.
- **Fix:** the name is now the check box's own `text`, so the whole `[ ] bit rate` is one target, with the hand cursor.
- **Also fixed:** `TyCheckBox` counted the `[x]` twice in its width when it had text (the label's left padding already holds it), which left wide gaps between items.
- Saving the choice on Windows was the separate preferences bug, fixed in the entry below.

**Files**
- `qml/GeneralSettings.qml`, `qml/controls/TyCheckBox.qml`
- `tests/tst_qml_smoke.cpp`

**Tests:** all 4 suites pass. `settingsGeneralLoads` clicks a column's name (not the box) twice, and checks that `Library.trackColumns` and the box follow each click.

### Preferences are saved on Windows (the CI failure) (2026-10-03, branch `abhinavp06/MEDIA_LISTING`)

**Summary:** `tst_media_models` had failed in the Windows workflow for several builds. With test output visible (`QT_FORCE_STDERR_LOGGING`, the entry below), the log showed three failures, all of one kind: a preference that was written read back as its old value (the category view, the scroll speed, the track columns).
- **Cause:** the bridge used `QSettings()`, which needs an organization name. TYLI sets only an application name. On Linux, Qt writes `~/.config/TYLI.conf` anyway; on Windows, it saves nothing (`AccessError`).
- **For the user:** on Windows, no preference was ever saved: sort choices, the quality remembered per album, movie, or show, track columns, the per-category view, and the scroll speed.
- **Fix:** preferences are `preferences.ini` in the data folder, next to `library.db` (`LibraryBridge::preferencesPath_`). The same file on every OS, `TYLI_DATA_DIR` moves it too, and each test now has its own.
- **Also fixed:** `preference(key, fallback)` returns the fallback's type. INI files keep only text, so "oldest first" (`false`) came back as the string `"false"`, which QML treats as true. That bug was there on Linux too.

**Decisions**
- **Rejected:** setting an organization name. `QStandardPaths::AppDataLocation` would then be `%APPDATA%\TYLI\TYLI`, and TYLI would open an empty library.
- **Rejected:** the registry. A file next to the database is easy to back up, inspect, and reset, and behaves the same on every OS.

**Tests:** all 4 suites pass.
- `TestMediaModels::preferencesPersistInTheDataFolderWithTheirTypes`: the file exists in the data folder, and after a restart a bool comes back as a `false` bool, a string as itself, and a category view as set.
- The three tests that failed on Windows now each read their own temporary file.

**Files**
- `src/library_bridge.{hpp,cpp}`
- `tests/tst_media_models.cpp`, `tests/tst_qml_smoke.cpp` (a comment)
- `DOCUMENTATION/application/ARCHITECTURE.md` (§3 and a decision row)

**Notes**
- **Preferences start fresh, once:** on Linux (this VM), the ones in `~/.config/TYLI.conf` are left behind. On Windows there were none.
- **Not the cause:** the suspected 27-second file-time shift on Windows (MSVC's `clock_cast` counting leap seconds). It's still unverified, and is listed in the workspace TODO.

### The user's next test: scroll speed setting, hand cursor, home page, no Books, white track icons (2026-10-03, branch `abhinavp06/MEDIA_LISTING`)

**Summary:** The user's notes from testing `cc88672` on Windows, and their answers:
1. **"Scroll speed is still slow":** 2× by default (one notch = 144 px at 3 lines), plus a **"scroll speed" setting** (Settings → General → interface, 50–500%). The bridge's `scrollSpeed` property stores it as `ui/scrollSpeed` and applies it at once; the bridge applies the stored value at startup. `WheelSpeed` (a constant) became `setWheelSpeed()` / `wheelSpeed()` in `app_setup`.
2. **Like and add-to-playlist icons:** white outlines (`Theme.text`). Liking itself comes with liked songs and playlists (the user: "only the look now"). The heart will fill with `Theme.accent`.
3. **The hand cursor:** the shared controls (`TyIconButton`, which covers the gear, plus `TyCheckBox`, `TyTab`, `TyPill`, `TyMenuItem`), the folder rows in Settings → Library, and the wordmark.
4. **No Books:** hoardor's file migration 3 removes the untouched default category (see its changelog). TYLI needed no code for it.
5. **`tyli_` opens a home page:** blank for now (designed after the player and playlists, and the place for recommendations). Back, a category, or a search result leaves it.
- **Also fixed:** in Settings → General, long help text ran under the fields and checkbox since the bigger sizes. Its column now takes only the width left over and wraps.

**Files**
- `src/app_setup.{hpp,cpp}` (`setWheelSpeed`, `wheelSpeed`, `DefaultScrollSpeedPercent`), `src/library_bridge.{hpp,cpp}` (`scrollSpeed`, `defaultScrollSpeed`)
- `qml/GeneralSettings.qml` (the interface section; `NumberSetting` takes a UI preference too; help text wraps)
- `qml/controls/{TyIconButton,TyCheckBox,TyTab,TyPill,TyMenuItem}.qml`, `qml/LibrarySettings.qml`
- `qml/TopBar.qml` (the wordmark), `qml/Main.qml` (`homeOpen`, `openHome`)
- `qml/TrackList.qml`
- `tests/tst_media_models.cpp`, `tests/tst_qml_smoke.cpp`
- `DOCUMENTATION/application/{ARCHITECTURE.md, features/theme.md, features/media_listing_ui.md}`

**Tests:** all 4 suites pass.
- `TestMediaModels::scrollSpeedIsAPreferenceAppliedAtOnce`: default 200%, applied at once, clamped to 500, and applied again by a new bridge (the next start).
- The smoke test's wheel test follows the setting (2×).
- `TestQmlSmoke::theWordmarkOpensHome`: a real click on `tyli_` shows the blank home, and the mouse's back button leaves it.
- A screenshot of the lower General page (`settings-general-lower`).
- The views test adds its own Books category.

**Not done:** liking, and the home page's design.

**CI: test output on Windows.** `tst_media_models` has failed in the Windows workflow for several builds, and the log showed no output at all. Without a console window, Qt Test and `qWarning` write to the Windows debugger (`OutputDebugString`), not to stdout. `tests/CMakeLists.txt` now sets `QT_FORCE_STDERR_LOGGING=1` for every suite, so the next run prints the `FAIL!` lines. The failure itself is still open: it doesn't reproduce on Linux.

### The user's Windows test: back and forward, scroll position, sizes, scroll speed, sidebar, posters (2026-10-03, branch `abhinavp06/MEDIA_LISTING`)

**Summary:** The user tested the Windows build on their real library and reported eight points. This entry covers TYLI's part. Item 7 (posters) was fixed in hoardor (see its changelog), and items 3 and 6 are deferred by the user.
1. **Scroll position on going back.** Returning from an album to the home grid scrolled to the top.
   - `CategoryView` now keeps the pages you came from alive in a `StackView`, so the grid comes back exactly as it was left: same object, same scroll position, rows already loaded.
2. **Mouse side buttons.** Back and forward now work like a browser:
   - the mouse's back/forward buttons, Alt+← / Alt+→, and keyboards' Back/Forward keys
   - forward history: a breadcrumb jump counts as several steps back, and a new page clears forward
   - back first closes Settings, then clears the search results
   - clicking the current category in the sidebar returns to its home page, as you left it
4. **Scrolling 20% faster.** One app-wide event filter (`setupApplication()`) scales the wheel's delta by `WheelSpeed` = 1.2 where the event reaches a window.
   - Measured in Qt 6.12: one notch moves lines-per-notch × 24 px (72 px at 3 lines), and 1.2× the delta moves 86 px.
   - Every list and grid speeds up alike, and Windows' own lines-per-notch setting still counts.
5. **No file counts in the sidebar.**
8. **Bigger text and icons, for the user's 27" 1440p and 15" 1080p screens.**
   - **One scale:** `Theme.scale` (1.15) and `Theme.px()`, through which every length in the QML now goes: base text 13 → 15 px, small 11 → 13, heading 15 → 17, title 18 → 21; icons 16 → 20; icon buttons and fields 32 → 36 px (just under the play button's 40).
   - **Unchanged on purpose:** `PlayButton` and `QualityPicker` (the user likes them).
   - **Different screens:** handled by Windows' per-monitor display scaling, which Qt 6 follows (and rescales live when the window changes screens). TYLI doesn't override it.
- **Deferred by the user:** album grouping (3: "much better now", wait for daily use) and the album count vs Plex (6: 4,557 vs about 1,834). Item 6 is written up in `features/media_listing_ui.md` §8 with the user's idea: an album is the folder that holds the audio files.

**Decisions**
- **A `StackView` that keeps pages alive, rather than saving `contentY` and restoring it.** A restored offset deep in a paged list would first need every page above it fetched again. Keeping the page costs only its models, and the stack is as deep as the pages opened.
- **Each page gets its own `route` and `depth` at creation.** Pages used to read `owner.route`, so a hidden page would have re-filtered itself every time a new page opened.
- **The side buttons go through a `TapHandler` on the window.** Since Qt 6.9, `Flickable` only accepts the left button (`acceptedButtons`), and no control takes the side buttons, so every side click reaches the window. A C++ event filter wasn't needed.
- **Sizes: no "UI size" setting.** The user: "I want something that just works… I don't want to do it every time I switch screens". Sizing from the monitor's physical size was rejected too: it fights the user's Windows scaling, and some monitors report wrong sizes.
- **The wheel speed is a constant** (`WheelSpeed`), for the same reason. Filtering only at the window keeps it from being applied twice: Qt Quick sends the event on to items with `sendEvent`, which app-wide filters see as well.

**Files**
- **Navigation:** `qml/CategoryView.qml` (rewritten: `routes`, `forwardRoutes`, `open`, `back`, `forward`, `goToCrumb`, `home`, `crumbs(depth)`, `homeKey`), `qml/Main.qml` (`goBack`, `goForward`, `clearSearch`, the `TapHandler`, two `Shortcut`s, the sidebar's current-category click).
- **Pages** (their own `route` and `depth`): `qml/{MusicView, AlbumView, ArtistView, MusicFilterView, VideoGridView, MovieView, ShowView, PersonView}.qml`.
- **Sizes:** `qml/Theme.qml` (`scale`, `px()`, new sizes); every QML file with a length (`Theme.px(…)`): `AlbumBrowser`, `BrowseHeader`, `CoverCard`, `CoverGrid` (cell height from `FontMetrics`), `CoverImage`, `GeneralSettings`, `HeroHeader`, `LibrarySettings`, `SearchView`, `SettingsView`, `ShowView`, `SortBar`, `StatusLine`, `TopBar`, `TrackList`, and `controls/{TyCheckBox, TyField, TyMenu, TyMenuItem, TyPill, TyProgress, TyScrollBar, TyTab}`. `qml/PlayButton.qml` and `qml/QualityPicker.qml` are pinned at their old sizes.
- **Sidebar:** `qml/Sidebar.qml` (no counts; the icon stays put when collapsed).
- **Wheel and app font:** `src/app_setup.{hpp,cpp}` (`WheelSpeedFilter`, `WheelSpeed`, the app font at 15 px).
- **Tests:** `tests/tst_qml_smoke.cpp`.
- **Docs:** `DOCUMENTATION/application/{ARCHITECTURE.md, features/media_listing_ui.md, features/theme.md}`.

**Tests:** all 4 suites pass, and the app starts offscreen without QML warnings. The smoke test has 3 new cases:
- `backKeepsThePageAndItsScrollPosition`: a small window so the grid scrolls; open an album and go back. The same grid object comes back at the same `contentY`. Also covers forward, a breadcrumb jump home, the sidebar's current category, and a new page clearing forward.
- `mouseSideButtonsGoBackAndForward`: real `BackButton` / `ForwardButton` clicks over the page and Alt+←/→. From Settings, back returns to the page.
- `theWheelScrollsAFifthFurther`: one notch on a plain `ListView` moves lines × 24 × 1.2 px.
- **The test helpers:** `find()` prefers a visible object, since hidden pages share object names. `showCategory()` returns to the category's home.

**Known limitations and follow-ups**
- History doesn't span categories: switching the category starts at its home, as before.
- Forward re-creates the page, so it opens at the top.
- Hidden pages keep refreshing their models while tags are read (small: one per page opened).
- The HTML mockups are still drawn at the old sizes.
- **To check on Windows:** both screens, the side buttons on the user's mouse, and the scroll feel.

### Search bar (2026-10-02, branch `abhinavp06/MEDIA_LISTING`)

**Summary:**
- **The field:** a search field in the top bar (debounced 200 ms; Esc clears).
- **While there's text,** a results page replaces the content: albums, artists, tracks, movies, and shows across every category, using hoardor's new `search` filter.
- **Opening a result:** it switches to its category and opens the album, artist, movie, or show page. A track opens its album, since playing comes with the player.
- **Matching:** every word as a prefix, accents ignored ("bjork" finds Björk).

**Added / changed**
- `qml/SearchView.qml` (new)
- `qml/TopBar.qml` (the field), `qml/Main.qml` (`searchText`, the results page, opening a result)
- `src/media_fields.cpp` (`search`), `src/library_bridge.*` (`categoryOfFolder`)
- `CMakeLists.txt`

**Tests:** the smoke test searches "radio" (albums, artist, tracks), "arri" (a movie by prefix), and "zzzz" (nothing), with screenshots. All 4 suites pass.

**Note:** this was built without a mockup round, because the user asked for everything to be finished in one go. The screenshot is in `build/screenshots/search.png`.

### Albums appear while tags are read; quality choices kept and remembered (2026-10-02, branch `abhinavp06/MEDIA_LISTING`)

**Summary:** This answers the user's request to see albums as their tags are read, and their worry about copies changing during playback.
- **Live updates:** the bridge emits `contentChanged` at most every 2.5 s during a metadata pass, and rebuilds thumbnails every 15 s.
- **`GroupModel::refresh()`:** re-reads the loaded range and inserts, updates, moves, or removes rows, never a reset, so grids fill in where you are.
- **Album, movie, and show pages:** keep the picked quality across refreshes and remember it per item. An album page mid-read says "reading tags · this album may still grow".
- **Rule for the player:** it queues specific files, so new copies never change what's playing (`features/media_listing_ui.md` §7b, ARCHITECTURE decision row).

**Files**
- `src/group_model.{hpp,cpp}` (`refresh`, `keyOf`, `page(cursor, error)`)
- `src/library_bridge.{hpp,cpp}` (`liveRefresh_`, thumbnail passes during reading)
- `qml/AlbumView.qml`, `qml/MovieView.qml`, `qml/ShowView.qml`
- tests and docs

**Tests**
- `tst_media_models.newContentIsInsertedInPlace`: no reset, one insert in sorted place, a grown album updated, the total right.
- The smoke test checks that a picked quality survives a refresh.
- All 4 suites pass.

### "Drives at a time" setting (2026-10-02, branch `abhinavp06/MEDIA_LISTING`)

**Summary:** hoardor now syncs and reads tags with one worker per physical drive. Settings → General → sync gains "drives at a time" (`parallelDevices`, 1–16, default 4; 1 = one drive at a time).

**Files**
- `src/library_bridge.cpp` (settings map, apply, limits)
- `qml/GeneralSettings.qml`

### Reading tags is its own background state (2026-10-02, branch `abhinavp06/MEDIA_LISTING`)

**Summary:** hoardor now reads metadata as a separate job after the sync (see its changelog). TYLI follows:
- **`Library.syncing`** is true only during the sync itself.
- **`Library.reading`** (new) is true while tags are being read.
- **`metadataProgress`** gains `perSecond`.
- **The top bar** shows "reading tags · 1,204 / 14,902 · 85/s" with a filling progress line once the sync is done. A new sync request pauses the reading, and it resumes afterwards.

**Files**
- `src/library_bridge.hpp/.cpp` (`reading`, `setReading`, `checkIdle` polls `syncing()` / `reading()`)
- `qml/TopBar.qml`

**Tests:** all 4 suites pass.

### Media library UI v1 finished: Settings controls and docs (2026-10-02, branch `abhinavp06/MEDIA_LISTING`)

**Summary:** This completes the work in progress from the entry below.
- **Settings → Library:** a "view" choice per category (automatic / music / movies / shows).
- **Settings → General:** "track columns" (length and bit rate by default).
- **Docs:** `features/media_listing_ui.md` has its as-built section, and `CLAUDE.md` lists the new sources.

**Files**
- `qml/LibrarySettings.qml`, `qml/GeneralSettings.qml`
- `DOCUMENTATION/application/features/media_listing_ui.md`, `CLAUDE.md`, `DOCUMENTATION/CHANGELOGS/unreleased.md`

**Tests:** all 4 suites pass. The smoke test's Settings screenshots show the new controls.

**Left:** the user's check on Windows with the real library.

### Media library UI v1: models, copy grouping, thumbnails, and every screen (work in progress) (2026-10-02, branch `abhinavp06/MEDIA_LISTING`)

**Summary:** TYLI's side of Media library v1, built on hoardor's audio and video engines.
- **C++:**
  - `GroupModel` / `ItemModel`: generic, paged models over hoardor's queries
  - `copy_grouper`: quality options, best first
  - `ThumbnailCache`: 300 px JPEGs made after each metadata pass; art can come from embedded pictures, or an image next to any of an album's tracks
  - `media_fields`: QML field names
  - bridge: metadata progress, `contentChanged`, album/movie/show copies, category views, preferences, track columns, `notice`
- **QML:** music (albums sorted by added ↓/↑, a–z, by artist, year), album, artist, genre, year, movies, movie, director, shows, and show pages. Shared sort bar, header, cover grid, quality picker, track list (disc headings, columns from preferences).
- **Removed:** `CategoryFilesModel` and its test (the old flat file list).

**Tests:** 4 suites passing on Linux.
- `tst_copy_grouper` (6 tests)
- `tst_media_models` (7 tests), including the fix for a total that read 0 when a small list ended during the model reset
- `tst_library_bridge`
- `tst_qml_smoke`: every new page, on seeded metadata, with screenshots

**Left:**
- Settings: the track-columns section and the per-category view choice
- the feature doc's as-built section
- the Windows check with the real library

### Media library UI designed (draft v2): metadata-driven views, copies and quality, new mockups (2026-10-02, branch `abhinavp06/MEDIA_LISTING`)

**Summary:** The user reviewed draft v1 (file-name lists). They asked for:
- real names from metadata, and one shared sort bar (added newest or oldest first, a–z, by artist, year)
- links for artist, year, genre, director, and drive
- a quality picker at the top right when an album, movie, or episode exists in several qualities (highest by default; grouping copies is TYLI's job)
- actions at the top right, configurable track columns (length and bit rate by default), and "CD 1" headings with a disc icon
- TODO markers for liked, playlists, click-to-play with a queue prompt, and viewed/progress
- no books

`features/media_listing_ui.md` is rewritten around generic models (`GroupModel`, `ItemModel`), a `CopyGrouper`, and a `ThumbnailCache`. Nothing is built yet.

**Changed / added**
- **Mockups:**
  - **Rebuilt:** `music-albums.html` (a working sort bar, including "by artist" headings), `music-album.html` (quality picker, the queue prompt on a track click), `movies.html`, `shows.html`, `show.html`.
  - **New:** `artist.html`, `genre.html` (it also serves years), `movie.html`, `director.html`.
  - **Removed:** `books.html`.
  - **`index.html`** lists them all.
- **`DOCUMENTATION/application/ARCHITECTURE.md`:** two decision rows (media library v1; UI rules).

**Files**
- `DOCUMENTATION/application/features/media_listing_ui.md`, `DOCUMENTATION/mockups/*.html`, `DOCUMENTATION/application/ARCHITECTURE.md`, `DOCUMENTATION/CHANGELOGS/unreleased.md`.

**Review round 2** (the user, 2026-10-02: "all good otherwise, we'll keep improving")
- "added" is one sort button that toggles newest and oldest first (↓ / ↑) on every grid.
- The album track list's header no longer carries an explanatory note.
- A year link opens a year page: `year.html` (music) and `movies-filter.html` (movies of a year or genre). Before, it opened the genre page.
- Round 3: on movies, the genre link opened the year page, and on shows, year and genre opened the shows grid. Each now has its own page: `movies-year.html`, `movies-genre.html`, `shows-year.html`, `shows-genre.html` (`movies-filter.html` removed).

**Follow-ups**
- The user's review is done. Next: the hoardor decisions in `features/media_listing.md` §8, then hoardor phase 1. TYLI's models and views follow hoardor's phases 1–4.

### Media listing UI designed: feature doc and mockups (draft, awaiting approval) (2026-10-02, branch `abhinavp06/MEDIA_LISTING`)

**Summary:** TYLI's side of media listing, which pairs with hoardor's `features/media_listing.md`. `features/media_listing_ui.md` designs a view per category layout:
- Music: an album grid, then an album with disc headings.
- Shows: a show grid, then season tabs and episodes.
- Movies and Books: lists.

It also covers the `MediaModel` / `FolderModel` models, a `CoverCache` that makes 300 px thumbnails right after each sync so browsing never wakes a drive, and the Settings additions. Six mockup pages show it. Nothing is built yet.

**Added**
- `DOCUMENTATION/mockups/`: `music-albums.html`, `music-album.html`, `shows.html`, `show.html`, `movies.html`, `books.html`.
  - The pages are marked "proposal · media listing (not built)" with a page switcher.
  - The covers are gradient stand-ins for the cached thumbnails.
  - `index.html` is now a list of pages, built and proposed, instead of a redirect to `music.html`.

**Decisions proposed to the user** (the doc's §7)
- Thumbnails with Qt in TYLI.
- Grids only for albums and shows.
- Movie posters and a subtitle indicator later.

**Files**
- `DOCUMENTATION/application/features/media_listing_ui.md` (new), the mockups above, `DOCUMENTATION/CHANGELOGS/unreleased.md`.

**Follow-ups**
- The user's review. Then the bridge and models, after hoardor's phases 1 and 2.
