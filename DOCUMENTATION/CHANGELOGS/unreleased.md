# Unreleased

Work that is done but not yet part of a version. The newest entries come first.

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
