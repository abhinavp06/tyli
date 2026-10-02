# Unreleased

Work that is done but not yet part of a version. The newest entries come first.

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
