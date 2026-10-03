# Unreleased

Work that is done but not yet part of a version. The newest entries come first.

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
