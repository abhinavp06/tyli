# Feature: Media library UI (v1)

| | |
|---|---|
| Status | **Draft v2, awaiting the user's approval** (2026-10-02). Reworked after the user's review of draft v1 (file-name listing). The mockups are ready, and nothing is built yet |
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

## 8. Open items

- **Playlists and liked songs:** their own feature (the columns are TODO).
- **Click-to-play, the queue prompt, viewed/progress markers:** the player.
- **Search:** later, on both sides.
- **Movie collections and franchises:** not planned.
