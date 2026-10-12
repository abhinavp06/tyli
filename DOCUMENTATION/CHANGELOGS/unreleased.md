# Unreleased

Work that is done but not yet part of a version. The newest entries come first.

### Shows, specials, and extras (with hoardor's "Shows, specials, and extras from release layouts") (2026-10-12, branch `abhinavp06/PLAYLISTS`)

**Summary:** The user's Windows test: "some of the TV shows are not showing up, even on a rescan". hoardor read their episodes as movies, and the Shows grid lists only episodes. Checked against the user's real folder lists, from a new read-only script. The fix is in hoardor: more episode names, cleaned show names, extras as a new video type, a show's films as its specials, and samples skipped. TYLI's side is small.

**What changed:**
- **Specials:**
  - A show's films, OVAs, and specials arrive as season 0, numbered 0 ("Specials" on the show page, as before).
  - `copies::groupShow` keyed an episode by (season, number), so they would have merged into one episode with dozens of "copies". Numberless episodes are now told apart, and ordered, by their case-folded title. The same title twice is still two copies.
- **Extras** (hoardor `video::Type::Extra`, 3):
  - The grids and pages already filter type 1 or 2, so featurettes, trailers, and samples no longer show up as movies.
  - The thumbnail pass grouped videos by title and by show without a type, so it would have made a poster, or grabbed a frame from the drive, for each of hundreds of extras. It now asks for movies and episodes only (titles of types 1 and 2, shows of type 2).
- **`tools/list_tree.ps1` (new):** lists every folder and file under a folder, with sizes, into a text file on the Desktop. It's read-only, uses `-LiteralPath` (release names are full of `[` `]`, which PowerShell treats as wildcards), writes UTF-8, and lists unreadable folders at the end. The user ran it on Shows, Movies, and Music. Those lists also confirmed the album folder rule (about 1,852 album folders vs Plex's about 1,834; `PRODUCT.md` I-11).
- **`DOCUMENTATION/PRODUCT.md` (new, 2026-10-04, the user's request):** the vision, the product principles, the release plan, and every idea the user has had (`I-<n>`) with its status and links. `CLAUDE.md` reads it at session start.

**Decisions (the user, 2026-10-12):**
- Extras go in an **Extras section on their show's or movie's page**, not in the grids.
- A show's films and OVAs are its **Specials**.
- **Samples** are never listed.
- The Extras section isn't built yet: a mockup comes first (`PRODUCT.md` I-18).

**Tests:** `tst_copy_grouper.specialsWithoutANumberStayApartByTitle` (two films apart, the same film twice as copies, ordered by title, a real episode after them); all 5 suites pass.

**Files:**
- `src/{copy_grouper.cpp, thumbnail_cache.cpp}`, `tests/tst_copy_grouper.cpp`, `tools/list_tree.ps1` (new)
- `CLAUDE.md`
- `DOCUMENTATION/{PRODUCT.md (new), application/ARCHITECTURE.md, application/features/media_listing_ui.md (§7h)}`

**Follow-ups:**
- The Extras section (mockup, then QML).
- The folder rule for albums and shows (TODO E).

### The video page's queue button sits right of the subtitles (2026-10-03, branch `abhinavp06/PLAYLISTS`)

**Summary:** The user asked to move the queue button "to the right of the subtitles options, or to the left of the volume". The controls now read: audio, subtitles, queue, mute, volume, full screen (it was first in that group).

**Files:** `qml/VideoPage.qml`

### Playlists and Liked Songs: the screens (2026-10-03, branch `abhinavp06/PLAYLISTS`)

**Summary:** The UI for the approved design (mockups `playlist.html`, `liked.html`; the user's review in `features/playlists_ui.md` §0), on the bridge from the previous commit and hoardor `799e939`. As built: §6.

**What changed:**
- **The sidebar's "playlists" section:**
  - **+** opens a name field with ✓ and ×; the new playlist opens.
  - Liked Songs is fixed on top, then the pinned playlists and the rest, each group in the user's order. Drag a row to reorder it within its group; a green line shows where it lands.
  - **On the right:** the green speaker while playing from it (`Player.source`), and the pin (on hover; always on pinned ones).
- **The playlist page** (top level, like the queue page):
  - **Hero:** a mosaic of its first four albums' covers; the name with a pencil that renames in place (✓ and × inside the field); "N tracks · length · updated"; delete with an inline "delete? / keep"; play.
  - **Rows:** drag by the handle that replaces the number on hover; artist and album links; heart; add to another playlist; × to remove. A click plays the playlist from that row (queue prompt). The playing row is marked.
- **Liked Songs:** the same page, with a heart cover, a "liked" column, and no editing.
- **Adding:** one add menu ("new playlist…" with a name field, then the playlists with checks; already-in ones can't be picked), opened from:
  - an album's track rows (the playlist icon is live now)
  - playlist rows
  - the now-playing bar (songs only)
  - the queue page's new "save as playlist"
  - No "add a whole album", per the user.
- **Shared parts:** `NameField` (a field with ✓ and × inside) and `PlaylistMenu`. The queue prompt passes the source on.

**Found on the way:**
- **The empty sidebar:** the category list was a `ListView` sized to its own content height, which with the new layout created no rows (the playlists section rose over "music"). Now a `Repeater`. The add menu's list had the same problem (now a `Column` in a `Flickable`).
- **Stale page info:** switching the page from Liked Songs to a playlist reloaded Liked Songs' info, because the change handler ran before the `liked` binding updated. `refresh()` reads `playlistId` itself.
- **Missing page margins:** the playlist page had none at first (the cover touched the sidebar, the play button the window's edge); it now has the queue page's.

**Tests:** 5 suites pass. `tst_qml_smoke::playlistsEndToEnd` (real clicks):
- create from the sidebar
- add through an album row's menu (a screenshot of the menu)
- the page with three rows
- rename with ✓, pin from the sidebar
- drag the third row to the top, × removes one
- play: the speaker lights
- Liked Songs
- delete with its confirm

The `showCategory` helper leaves a playlist page, as a sidebar click does; one test finds the sidebar by name. Screenshots: `playlist`, `playlist-add-menu`, `playlist-rename`, `playlist-playing`, `liked-songs`.

**Files:**
- `qml/{Sidebar.qml, PlaylistPage.qml (new), PlaylistMenu.qml (new), NameField.qml (new), Main.qml, QueuePrompt.qml, QueuePage.qml, TrackList.qml, PlayerBar.qml, Icon.qml}`
- `src/{player_bridge.hpp, playlist_model.hpp, playlist_model.cpp}`, `CMakeLists.txt`, `tests/tst_qml_smoke.cpp`
- `DOCUMENTATION/application/{features/playlists_ui.md, ARCHITECTURE.md}`

**Known limitations:**
- Reordering drags rows already loaded (pages of 200) and doesn't scroll by itself.
- A moved or renamed file drops out of playlists (no move detection yet).

### Playlists: the bridge and `PlaylistModel` (2026-10-03, branch `abhinavp06/PLAYLISTS`)

**Summary:** TYLI's side of hoardor `799e939` (playlists in `player::Library`). The screens follow in the next commit.

**What changed:**
- **`Player` (the bridge)** gets the playlist calls. It already holds `player::Library` for likes, so this replaces the design's "on `Library`" (doc updated).
  - **Properties:** `playlists` (in the sidebar's order, refreshed on every change and on `Library.contentChanged`) and `source`.
  - **Calls:** `createPlaylist` (−1 with the reason on the status line), `renamePlaylist`, `deletePlaylist`, `setPlaylistPinned`, `movePlaylist`, `addToPlaylist`, `removeFromPlaylist`, `movePlaylistItem`, `playlistInfo`, `playlistEntries`, `playlistsWith`, `likedEntries`, `likedInfo`.
  - **Status line:** "added 2 tracks to Road Trip · 1 was already in it", "removed 1 track", "deleted “Jazz”".
- **`Player.source`:** set by `playNow(entries, start, source)`; another `playNow` without one, or `clear`, empties it. Deleting the playlist that's playing empties it too. The sidebar's speaker follows it.
- **`PlaylistModel` (new):** a playlist's rows, or Liked Songs (`playlistId` −1).
  - Keyset pages of 200 from hoardor, each row's track looked up in the audio engine: title, artist, album, length, online, liked, when added or liked.
  - It reloads on `playlistsChanged`. Liked Songs also reloads on likes; a playlist's hearts update in place.
  - `entryIds()` is every row (to play it all).

**Tests:** 5 suites pass. `tst_player_bridge::playlistsThroughTheBridge`:
- create, a taken name, the order, pinning
- adds with skips, `playlistsWith`
- the model's rows and artists, moves and removes, rename
- Liked Songs following the hearts, newest first
- `source` set and cleared
- delete

**Files:** `src/{player_bridge.hpp, player_bridge.cpp, playlist_model.hpp (new), playlist_model.cpp (new)}`, `CMakeLists.txt`, `tests/tst_player_bridge.cpp`, `DOCUMENTATION/application/features/playlists_ui.md`

### Shows: a season plays as a queue you can see from the video page (2026-10-03, branch `abhinavp06/PLAYLISTS`)

**Summary:** The user: "while playing a season of a show, I want the same queue system like an album/playlist".
- **Already true:** the season is queued (decision P2).
- **The gap:** the queue could only be seen on the queue page, and leaving the video page pauses the video.

**What changed:**
- **`QueuePanel.qml` (new):** the expanded player's queue panel as its own component ("queue · N next · length", clear, ×, the rows). Used by `ExpandedPlayer` (unchanged for the user) and now by the video page.
- **`VideoPage`:** a queue button (and `q`) opens the panel on the right, above the controls, which stay up while it's open.
  - A click on a row jumps to that episode; × removes one.
  - With an episode playing, rows read "S1 E2 · title" (`QueueList.episodes`), since every row's title would be the show's name.

**Tests:** `tst_qml_smoke::playingMusicAndVideo`: the panel opens with the season's 3 episodes and closes. The expanded player's panel tests pass unchanged. Screenshot: `video-episode`.

**Files:** `qml/{QueuePanel.qml (new), ExpandedPlayer.qml, QueueList.qml, VideoPage.qml}`, `CMakeLists.txt`, `tests/tst_qml_smoke.cpp`

### Shows: previous and next episode on the video page (2026-10-03, branch `abhinavp06/PLAYLISTS`)

**Summary:** The user: "add next episode and previous episode buttons, only for shows".

**What changed:**
- **`VideoPage`:** while an episode plays (`Player.link.page` is "show"), "previous episode" and "next episode" sit around play/pause; `p` and `n` work too. They walk the season's queue (`Player.jump`; an episode queues its season, decision P2), and are disabled at its first and last episode.
  - "Previous" goes straight to the episode before; it doesn't restart this one first, as music's "previous" does after 3 s.
  - A movie shows neither.
- **The smoke test's sample library:** two Dark episodes are real clips now (copies of Heat's), so a season really plays.

**Tests:** `tst_qml_smoke::playingMusicAndVideo`:
- a movie: no episode buttons
- season 1 from its page: a queue of 3; previous disabled on the first, next enabled; next goes to episode 2, previous back to episode 1

Screenshot: `video-episode`.

**Files:** `qml/VideoPage.qml`, `tests/tst_qml_smoke.cpp`

### The full-screen icon follows the window (2026-10-03, branch `abhinavp06/PLAYLISTS`)

**Summary:** The user: when the window goes full screen or back, "the fullscreen icon should change accordingly; right now it stays the same".

**What changed:**
- **The video page's button** shows "full screen" (corners out) or "leave full screen" (corners in, the new `fullscreenExit` icon).
- **`Main.qml`:** `fullScreen` follows the window both ways.
  - The button sets the window's visibility.
  - A window that changes by itself (the OS, a shortcut) sets `fullScreen`. Minimizing doesn't count as leaving.
- **Leaving full screen** restores the window as it was: maximized stays maximized (before, it always became a normal window).

**Tests:** `tst_qml_smoke::playingMusicAndVideo`: the button and its icon; the window leaving and entering full screen by itself; back again.

**Files:** `qml/{Main.qml, VideoPage.qml, Icon.qml}`, `tests/tst_qml_smoke.cpp`

### Settings → About names the author (2026-10-03, branch `abhinavp06/PLAYLISTS`)

**Summary:** The user asked for their name in About: "tyli x.x.x · by abhinavp06". The About line now reads "tyli 0.3.0 · by abhinavp06" (the version is the app's, from CMake).

**Tests:** `tst_qml_smoke::settingsGeneralLoads` checks the line.

**Files:** `qml/GeneralSettings.qml`, `tests/tst_qml_smoke.cpp`
