# Feature: Media library UI (v1)

| | |
|---|---|
| Status | **Shipped in `v0.2.0`** (2026-10-03), with hoardor `v0.2.0`; tested by the user on Windows with their library (§7d, §7e). Open: the album count vs Plex (posters for most movies and shows: §7f, waiting for the user's test), the home page's design. Design approved 2026-10-02 (draft v2) |
| Branch | `abhinavp06/MEDIA_LISTING` (pairs with the hoardor branch of the same name) |
| Ships in | TYLI `v0.2.0`, together with hoardor `v0.2.0` |
| hoardor feature | Media library v1: `../hoardor/DOCUMENTATION/application/features/media_listing.md` (metadata with ffmpeg, the `audio`/`video` engines, generic filter/order/group queries) |
| Mockups | `DOCUMENTATION/mockups/index.html` → "proposal · media library": music, an album, an artist, a genre, movies, a movie, a director, shows, a show |

## 1. What the user asked for (review of draft v1, 2026-10-02)

- **Real names from metadata,** shown exactly as tagged. The UI's own labels stay lowercase, as in the theme.
- **Every grid has the same sort bar,** built once in TYLI and reused: **added** (one button: the first click sorts newest first, the next click flips to oldest first, shown as ↓ / ↑), alphabetical, by artist (grouped under headings, music only), and year.
- **Links everywhere:** artist, year, genre, director. Each opens a page of that artist, year, genre, or director. The drive indicator links to Settings → Library.
- **Copies in different qualities are TYLI's job:** an album at 16/44.1 and 24/96, or a movie in 1080p and 4K, appears once, with a **quality picker at the top right** of its page. The highest quality is selected by default.
- **Actions live at the top right** (play, quality): a general rule for TYLI from now on.
- **Track columns are a global setting:** by default length and bit rate; optionally size, type, sample rate / bit depth, and more.
- **Discs show as "CD 1", "CD 2"** headings with a disc icon.
- **Later, shown as TODO in the mockups:**
  - liked (heart) and playlists columns
  - click-to-play with the queue prompt
  - viewed and progress markers on movies and episodes (a check, and a progress line inside the name, not a separate column)
- **No folder views.** Only thumbnails and the layouts.
- **Episodes show names** ("name", never "file"), with no file extensions.
- **No books or other text:** a separate, big build.

## 2. Screens

| Screen | Mockup | Built from (hoardor, generic) |
|---|---|---|
| **Music** (album grid) | `music-albums.html` (the sort bar works) | `audio::groups({AlbumArtist, Album}, {Category})`. The sort maps to `AddedLast` (descending or ascending), `Keys` on the album, `Keys` on album artist then album with TYLI drawing artist headings, or `Year` |
| **Album** | `music-album.html` | `audio::tracks({AlbumArtist, Album}, {Disc, Track})`. TYLI groups copies by (disc, track) into quality options: `24/96 FLAC`, `16/44.1 FLAC`, `MP3 320`. A partial copy shows as "9 of 11 tracks" |
| **Artist** | `artist.html` | Albums: `groups({AlbumArtist, Album}, {AlbumArtist=X})`. Appears on: `groups(…, {Artist=X})` minus their own albums |
| **Genre** | `genre.html` | `groups({AlbumArtist, Album}, {Genre=X})`, with the same sort bar |
| **Year** | `year.html` | `groups({AlbumArtist, Album}, {Year=Y})`. Its own page (review round 2): a year link never opens the genre page |
| **Movies of a year / of a genre** | `movies-year.html`, `movies-genre.html` | `video::groups({Title, Year}, {Type=movie, Year=Y})` / `{Genre=X}` |
| **Shows of a year / of a genre** | `shows-year.html`, `shows-genre.html` | `video::groups({Show}, {Type=episode, Year=Y})` / `{Genre=X}` |
| **Movies** (poster grid) | `movies.html` | `video::groups({Title, Year}, {Type=movie, Category})`: one card per movie, with its best quality under the title |
| **Movie** | `movie.html` | `video::items({Title, Year})`: the copies feed the quality picker (`2160p · HDR10 · HEVC` vs `1080p · SDR · H.264`), plus genres, directors, runtime, and the plot from the `.nfo` |
| **Director** | `director.html` | `groups({Title, Year}, {Director=X, Type=movie})`, plus "episodes" for shows they directed |
| **Shows** (poster grid) | `shows.html` | `video::groups({Show}, {Type=episode})` |
| **Show** | `show.html` | Season tabs: `groups({Show, Season}, {Show=X})`. Episodes: `items({Show, Season}, {Episode})`. Copies per episode give the show-wide quality picker ("18 of 26 episodes" when a quality is partial) |

- **Opening a page** replaces the content area, with a back arrow and a breadcrumb. There are no new windows.
- **Covers and posters** come from TYLI's thumbnail cache only. A missing thumbnail shows a placeholder icon, and offline items are dimmed.
- **The top bar** shows metadata reading after a sync ("reading tags · music (e:) · 1,204 / 14,902") with the same progress line as sync.
- **The queue prompt** (TODO with the player) is a deliberate exception to the theme's "no popups" rule, requested by the user: cancel / add to queue / clear queue and play.

## 3. Bridge and models (`src/`)

Generic models that mirror hoardor's generic queries. A screen configures them, so there's no model per screen.

| Class | What it is |
|---|---|
| **`GroupModel`** | `engine` (audio / video), `groupBy` (a list of field names), `filter` (a map of field to value), `sort`, `descending`. Paged with hoardor's cursor. Roles: `keys`, `count`, `duration`, `addedLast`, `yearMin`, `yearMax`, `online`, `cover` (a cached thumbnail URL or empty), plus convenience roles (`title`, `subtitle`) |
| **`ItemModel`** | `engine`, `filter`, `order`. Roles: every field of `audio::Track` / `video::Item` |
| **`CopyGrouper`** | Plain C++, no QObject. Groups an album's tracks or a movie's or show's items into copies, labels each quality, and orders them best first (lossless before lossy; then bit depth, sample rate, and bit rate for audio; then height, HDR, and codec for video). It's unit-tested on its own |
| **`QualityModel`** | The picker's options for one page, and the tracks or items of the selected copy |
| **`ThumbnailCache`** | A class: it holds a queue and a worker. After each metadata pass, on a `QtConcurrent` task with its own hoardor connection, it makes thumbnails for every group cover or poster missing one. The source is an embedded picture (hoardor's `embedded_cover` bytes) or a companion image file. They're scaled with `QImageReader::setScaledSize` (300 px for covers, 300×450 for posters), saved as `AppData/thumbnails/<entry id>-<size>-<mtime>.jpg`, and orphans are removed in the same pass |
| **`Library`** (existing bridge) | Gains `metadataProgress`, a `metadataRunning` flag, and settings for the new hoardor options |

**UI preferences** (`QSettings`, editable in Settings → General):
- track columns (default: length and bit rate)
- the sort per grid
- the thumbnail size

## 4. QML (`qml/`)

- **Shared components:**
  - `BrowseHeader.qml`: the title, counts, and sort bar, reused by every grid and page
  - `SortBar.qml`
  - `CoverGrid.qml`: square or poster cards, with optional section headings for "by artist"
  - `HeroHeader.qml`: the art, the label, the title, fact links, the drive link, and the top-right actions slot
  - `QualityPicker.qml`
  - `TrackList.qml`, with disc headings and columns from the setting
  - `EpisodeList.qml`
  - `LinkText.qml`: a dotted underline that turns green on hover
- **Screens:**
  - `MusicView.qml`, `AlbumView.qml`, `ArtistView.qml`, `FilterView.qml` (one component; a genre page and a year page are separate instances with their own titles and breadcrumbs)
  - `MoviesView.qml`, `MovieView.qml`, `PersonView.qml` (director)
  - `ShowsView.qml`, `ShowView.qml`
- **Which grid a category opens:**
  - The default comes from the category's accepted kinds: audio opens Music; video opens Movies or Shows, by what the items are (episodes or movies).
  - The user can override it per category in Settings → Library: view = music / movies / shows.
  - This is a TYLI preference stored in `QSettings`, not a hoardor field.
- **`CategoryView.qml` and `CategoryFilesModel`** are removed, replaced by the views above.

## 5. Edge cases

- **Metadata not read yet** (a fresh library or a big backlog): the grids fill in as reading progresses, and the top bar shows progress. Items read from names only are marked faint until real metadata exists.
- **Copies:**
  - A partial copy, for example an MP3 version missing two tracks, is labelled "9 of 11 tracks".
  - Two copies of the same quality on different drives: one option; the online one plays.
- **One album split across drives:** the cards merge (the same group key). An offline part is dimmed in the track list.
- **Albums without an album artist:** grouped under the track artist (hoardor's fallback). Compilations show under "Various Artists" when tagged that way.
- **A movie with the same title and year but different films:** a known limitation of grouping by (title, year). An `.nfo` id can disambiguate later.
- **A thumbnail fails to decode:** placeholder, not retried until the source changes.
- **Long names:** elided, with a tooltip; the album page's title wraps to 2 lines.
- **A sync or metadata pass while a page is open:** the models reload and keep the scroll position.
- **No category of a kind:** that grid isn't offered.

## 6. Tests

- **`tst_copy_grouper.cpp`:** quality labels and order (24/96 > 16/44.1 > MP3 320; 2160p HDR > 2160p > 1080p), partial copies, ties, missing tech fields.
- **`tst_group_model.cpp` / `tst_item_model.cpp`:**
  - paging, sorting, and filters against a temporary library
  - for metadata, the tests use hoardor's test helpers that write `audio_tracks` / `video_items` rows directly, so no media files are needed
- **`tst_thumbnail_cache.cpp`:** a generated PNG becomes a thumbnail of 300 px or less, unchanged sources aren't redone, orphans are removed, corrupt images fall back.
- **`tst_qml_smoke.cpp`:** every new screen loads offscreen without QML warnings, with screenshots and the pure-black check.

## 7. Phases

1. **Mockups:** v2 done 2026-10-02, waiting for review.
2. After hoardor's phases 1–4: `CopyGrouper`, the models, `ThumbnailCache`, and tests.
3. Shared components and screens, Settings additions, the smoke test.
4. The user's check on Windows with the real library (the GitHub Actions zip; ffmpeg's DLLs ship in it).

## 7a. As built (2026-10-02)

- **C++** (`src/`):
  - `media_fields` (QML field names → hoardor fields and filters)
  - `group_model`, `item_model`
  - `copy_grouper`, `thumbnail_cache`
  - the bridge: `albumCopies`, `movieCopies`, `showCopies`, `thumbnail`, `categoryView` / `categoryViewChoice` / `setCategoryView`, `preference` / `setPreference`, `trackColumns`, `metadataProgress`, and the signals `contentChanged`, `metadataFinished`, `notice`
  - `CategoryFilesModel` is removed.
- **QML** (`qml/`):
  - shared pieces: `CategoryView` (routing with a back stack and breadcrumbs; a `StackView` with forward since 2026-10-03, §7d), `BrowseHeader`, `SortBar`, `CoverGrid`, `CoverCard`, `CoverImage`, `HeroHeader`, `QualityPicker`, `PlayButton`, `LinkText`, `TrackList`
  - screens: `AlbumBrowser`, `MusicView`, `MusicFilterView` (genre or year), `AlbumView`, `ArtistView`, `VideoGridView` (movies or shows, also their genre and year pages), `MovieView`, `ShowView`, `PersonView` (director)
- **Settings:**
  - **Library:** under a selected category, "view: automatic · music · movies · shows".
  - **General:** "track columns": length, bit rate, bit depth / sample rate, file type, size, channels.
- **Deviations from §3–§6:**
  - **Album art:** the cache looks next to any of an album's tracks, not only the cover track. Copies of one album can sit in different folders, and only one may have `cover.jpg`.
  - **Thumbnail names:** `<entry>-<mtime>.jpg`, the mtime coming from the database, so a lookup never touches a drive.
  - **A grid's total** is counted before the model reset. A view fetches its first page during the reset, and a small list then ends at once; reading the end state after the reset made the total 0.
  - **Disc headings** are rows in the track list, because ListView sections don't work on JS arrays.
  - **List helpers:** lists from C++ have no `flatMap` in QML, hence `Format.flatten` and `Format.unique`.
  - **Not built yet (TODO in the UI):**
    - a track click shows a status-line notice instead of the queue prompt (the player)
    - liked and playlist icons are placeholders
    - viewed and progress markers come with the player
  - **Tests:** the planned `tst_thumbnail_cache` is part of `tst_media_models`.
- **Tests:** `tst_copy_grouper` (6), `tst_media_models` (7), `tst_library_bridge`, and `tst_qml_smoke`. The smoke test visits every new page over seeded metadata and checks the copies and the quality pickers.

## 7b. While tags are read; copies and playback (2026-10-02, the user's questions)

- **Progressive listing:**
  - During a metadata pass the bridge emits `contentChanged` at most every 2.5 s, and once at the end. Thumbnails are rebuilt every 15 s during the pass.
  - `GroupModel::refresh()` re-reads the loaded range and applies the differences (insert, update, move, remove), never a reset, so a grid fills in where you are instead of jumping to the top.
  - Files are read root by root in scan order (folder by folder), so an album usually completes within a second of its first track.
  - An album page opened mid-read says "reading tags · this album may still grow".
- **Copies and playback (rules):**
  - **The player plays specific files (entry ids), never "an album".** A copy found later changes what the album page offers, never what's playing or queued.
  - **The quality you pick is kept across refreshes and remembered** per album, movie, or show (`QSettings` `quality/…`). "Best by default" applies only when you haven't picked one.
  - When the player exists, the playing album's page will show which copy is playing.

## 7c. Search (2026-10-02)

- **The field:** the top bar's search field (debounced) shows `SearchView` in place of the page.
- **Sections:** albums (`GroupModel` grouped by category, album artist, album), artists, tracks (`ItemModel`), movies, and shows, each filtered with `search`; up to 12 shown per section.
- **A result** switches the sidebar to its category and opens its page there. A track opens its album.
- **Off the UI thread (2026-10-03, the user: "search feels slow"):** at 50k tracks one keystroke cost about 0.5 s of queries on the UI thread (hoardor's `BM_SearchKeystroke`), so typing froze the window.
  - `Library.search(text)` runs the five sections on Qt's thread pool, on its own connection, fetching only what the page shows (12 of each, 24 artists) and the totals.
  - Only the latest text is answered (`searchFinished`). A search overtaken while running is dropped, and the newest text runs next.
  - The page waits 400 ms after the last keystroke (was 200), and needs at least 2 letters ("type at least 2 letters").
- **Back returns to the results:** a page opened from a result is marked (`CategoryView.returnDepth`). Back from it, by the arrow, a mouse button, or Alt+←, shows the results again as they were (`returnRequested`). A crumb above that page, or another category, drops the way back.

### Search from anywhere, and a clear button (2026-10-03, the user)

- **An × inside the search field** while it has text (`clearSearch`): it clears the search, the page comes back at once (no typing delay), and the field keeps focus for the next search. Esc still clears too.
- **Search works from every page, Settings included.**
  - The results cover the page, Settings too (`view` puts search first).
  - Clearing them, or going back, shows the page again, on the same Settings page.
  - Opening a result leaves Settings.
  - The gear is lit only while Settings actually show; over results, it brings them back.

## 7d. Back, forward, and keeping your place (2026-10-03, the user's Windows test)

- **The user's findings:** going back from an album to the home grid scrolled to the top, and the mouse's side buttons did nothing.
- **Pages stay alive:** `CategoryView` keeps a `StackView` of pages, one per route. Opening a page pushes it, going back pops it, and the page underneath reappears exactly as it was: the same scroll position, rows already loaded, sort. Each page gets its own `route` and `depth` when it's created (it no longer reads the current route), so a hidden page never re-filters itself.
- **Forward**, like a browser: going back keeps the pages you left in `forwardRoutes`, a breadcrumb jump counts as several steps back, and opening a new page clears them. Forward re-creates the page (not kept alive).
- **Input:** the mouse's back/forward buttons (a `TapHandler` on the window: Qt Quick's lists only take the left button, so side clicks always reach it), Alt+← / Alt+→, and keyboards' Back/Forward keys. Back first closes Settings, then clears the search results, then walks the category's pages.
- **The sidebar:** clicking the category you're in goes back to its home page, as you left it.
- **Cost:** hidden pages keep their models, which also refresh while tags are read. The stack is only as deep as the pages opened, so it stays small.
- **Not done:** history across categories (switching the category starts at its home, as before).

## 7e. Home page, Books, track icons (2026-10-03, the user's next test)

- **`tyli_` opens the home page** (`Main.homeOpen`). It's blank for now: it gets designed after the player and playlists, and is where recommendations will go. A category, a search result, or back leaves it. The app still starts on the first category.
- **No Books:** hoardor no longer keeps an untouched Books category (its file migration 3), so the sidebar and Settings show Music, Movies, and Shows.
- **Track list actions:** white outline heart and playlist icons. Liking (the heart fills with the accent color) comes with liked songs and playlists; the user chose to do only the look now.

## 7f. Posters for every movie and show (2026-10-03, the user's choice)

Most folders have no art, because Plex keeps it in its own data folder. The user chose Plex's posters, with a frame from the video as the fallback. The design is hoardor `features/posters.md`.
- **`ThumbnailCache`** goes through, per movie or show:
  1. the poster image
  2. the poster inside the video
  3. **Plex's poster** (read-only, found automatically; `TYLI_PLEX_DIR` points elsewhere)
  4. **a frame** from the first video (a show's first real episode)
- **Frames only after tag reading** (`rebuild(false)` during it), because they read the drive. Plex's posters come from internal storage, so they're also looked up during tag reading.
- **Not retried in the same session:** a failed frame, and (while tags are read) a video Plex had nothing for.
- **The cover entry** is `fields::videoCover(group)`: poster, else embedded, else `first_entry`. The grid, search, and the player bar (its movie's or show's group) use it, so they all show the same picture.
- **`tools/plex_probe.ps1`** (also in the Windows zip) is a read-only check of Plex's layout on the user's PC, in case its posters don't show.
- **Tests:** `tst_media_models::videoPostersFromPlexThenFrames`, with a fake Plex, a real video, and junk:
  - Plex's poster in a pass without frames
  - the frame in a pass with them
  - a failed frame not retried
  - the grid's thumbnails

## 7g. Album structure for movies and shows (2026-10-03, the user; mockups approved)

The user asked for music's album structure in movies and shows. Mockups: `show-seasons.html`, `season.html`, `movie-album.html`, `movies-by.html`, `shows-by.html`. Built as separate commits:
1. **"By" sections in the grids:**
   - movies: "by director" and "by genre"; shows: "by genre", since an episode's directors vary
   - like music's "by artist": a section per name, a–z, each a link to its page, with its cards a–z (the first 120; the page has all)
   - a "no director" / "no genre" section last, so nothing drops out (hoardor `Condition::none`)
   - only on a category's own page; the choice is remembered with the other sorts
   - section counts sit on the name's baseline, music's included
2. **Seasons as album cards, and a season page in the album layout:**
   - **The show page** (`ShowView`): the hero (poster as tall as an album cover), the show's quality and play (the next episode), then a card per season, specials last.
     - A card shows the show's poster (hoardor keeps no season posters yet), "N episodes · year", and a green line for how much of the season you've watched; "watched" when all of it.
     - The season tabs and the episode list moved to the season page.
   - **The season page** (`SeasonView`, route `season`):
     - the album layout: poster, "season", its show (a link back), year, genres, directors, then "N episodes · runtime · M watched" and the drive
     - quality (that season's qualities) and play (resumes within the season)
     - its episodes as tracks, as before
   - **`ShowState`** is shared by both pages: the episodes and copies, the chosen quality (one per show), what you watched, and what plays next.
   - **The cover:** `Library.showCopies` returns the show's `coverEntry` (as on its card), so a page opened without one still shows it.
3. **The movie page in the album layout:** its copies as rows, then "more by" the director and "more in" each genre (below, as built).
4. **The folder rule:** waiting for the user's answers.

## 8. Open items

- **Playlists and liked songs:** their own feature (the columns are TODO).
- **Click-to-play, the queue prompt, viewed/progress markers:** the player.
- **Album count vs Plex** (the user, 2026-10-03, to revisit after daily use): TYLI shows about 4,557 albums where Plex shows about 1,834. Albums are hoardor groups of (album artist, album), and singles or loosely tagged tracks become albums of their own. Idea to try: an album is the folder that directly holds the audio files (the ARTIST/ALBUM/TRACKS layout), so counting those folders gives the real number.
- **Movie collections and franchises:** not planned.
