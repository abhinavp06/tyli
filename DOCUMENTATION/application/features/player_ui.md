# Feature: Player UI, first draft

| | |
|---|---|
| Status | **Built** (2026-10-03, §5a); waiting for the user's Windows test. Approved 2026-10-03 ("looks good", plus the like heart); mockups `player-*.html` |
| Branch | `abhinavp06/PLAYER` (pairs with hoardor's branch of the same name) |
| Ships in | TYLI `v0.3.0`, together with hoardor `v0.3.0` |
| hoardor side | `../hoardor/DOCUMENTATION/application/features/player.md` (the `player` engine on libmpv, the queue, progress) |
| Scope (the user, 2026-10-03) | Music: the queue prompt and a now-playing bar. Video: a player page. Resume and viewed markers. Later: bit-perfect output, playlists and liked songs, the home page |

## 1. Screens (mockups: `DOCUMENTATION/mockups/player-*.html`)

### 1.1 Music: click to play, the queue prompt (`player-music.html`)

- **Clicking a track:**
  - **nothing playing:** the album plays from that track, in the album's order, using the copy picked on the page
  - **something playing:** the **queue prompt** asks: *cancel / add to queue / clear queue and play*. Esc cancels, and Enter clears the queue and plays.
- **The album's play button** does the same, from track 1. An artist page's play button plays the artist's albums, newest first.
- **The hearts on track rows work now:** a click likes or unlikes the track, with the same look as the bar's (the user, 2026-10-03). The playlist icon next to it waits for the playlists feature.
- **The playing track** is marked on its album page: a green ▶ instead of its number, the title in green, and the green line on its left.
- **The prompt is the one deliberate popup** (ARCHITECTURE decision, 2026-10-02).

### 1.2 The now-playing bar (every page while something is loaded)

- **Left:** the cover, the title (a link to its album, or to the movie's page), and the artist · album · quality.
- **Middle:**
  - previous, play/pause (a 36 px green-bordered button, close to the play button the user likes), next
  - a seek line with the elapsed and total times; the buffered part shows faint behind it
- **Right:** ♥ like (a white outline; **green and filled when liked**, in `Theme.accent`, so it follows the theme), add to a playlist (the playlists feature, later), the queue (its own icon; it opens the queue page), mute, and a volume line.
- **Visibility:** hidden while nothing is loaded. It sits between the content and the status line.

### 1.3 The queue (`player-queue.html`)

- **A page in the content area,** like Settings: opened from the bar's queue icon; the icon or back closes it.
- **Rows:** number, title, artist · album, length. Played tracks stay above, dim. The current one is green.
- **Actions:**
  - click a row: jump to it
  - × on hover: remove it
  - **clear** (top right): stop and empty the queue
- **Later:** reordering by drag, and "play next".

### 1.4 The video page (`player-video.html`)

- **Opens** from a movie's play button, or a click on an episode.
- **The picture** fills the content area, with an **overlay** of controls:
  - **top:** back, then the title, year, quality, and drive
  - **bottom:** a seek line, play/pause, −10 s / +10 s, the time, the **audio** and **subtitles** menus (each track's language, codec, and title; external `.srt` files say "next to the file"), mute and volume, and full screen
- **Hiding:** the overlay and the cursor hide after 3 s without a mouse move.
- **Keys:** space pauses, ← / → seek 10 s, `f` toggles full screen, Esc leaves full screen, `m` mutes.
- **Resume:** "resumed at 42:10 · from the start" shows for a few seconds when playback resumes.
- **Leaving the page** (back, the sidebar, the side mouse button) **pauses** the video. The now-playing bar keeps it, and clicking its title returns to the page.
- **Full screen** hides the top bar, the sidebar, the now-playing bar, and the status line.

### 1.5 Viewed and progress (`player-markers.html`)

- **Episodes:**
  - **viewed:** a dim green ✓ in the last column
  - **partway:** a green line under the name, as long as the part watched
- **Movies:** the same on their cards: the ✓ in the poster's corner, the line under the poster. The movie page's play button says "resume at 42:10" in its tooltip.
- **The show's play button** resumes the episode you're partway through, or plays the next unwatched one.

## 2. Decisions (proposed, then approved by the user with the design, 2026-10-03)

| | Proposal | Why |
|---|---|---|
| P1 | **Playing a movie or episode replaces the queue,** with no prompt. The prompt is for music | A queue of music then a film is rarely wanted, and the prompt would interrupt "watch this" |
| P2 | **Playing an episode queues the rest of its season,** so the next one follows | Binge-watching without a playlist feature; it stops at the season's end |
| P3 | **Leaving the video page pauses** | Video playing behind other pages is easy to forget; music keeps playing everywhere |
| P4 | **Qt Quick draws with OpenGL on every OS** (`QQuickWindow::setGraphicsApi`), not Direct3D 11 on Windows | mpv's render API is OpenGL. Checked on the VM; to check in the Windows build (hoardor OI-P2) |
| P5 | **The queue is a page, not a side panel** | The same pattern as Settings; no overlapping panels |

## 3. Bridge and models (`src/`)

| Piece | What |
|---|---|
| **`PlayerBridge`** (QML singleton `Player`) | A second bridge object, because it holds state (the hoardor `Player`, its thread's callbacks). It forwards hoardor's callbacks with queued calls, like `SyncWorker`. **Properties:** `state`, `entryId`, `title`, `subtitle`, `coverEntry`, `hasVideo`, `position`, `duration`, `volume`, `muted`, `audioTracks`, `subtitleTracks`, `queueLength`, `current`. **Invokables:** `playNow(entryIds, start)`, `setLiked(entryId, liked)`, `add(entryIds)`, `toggle`, `next`, `previous`, `seek`, `seekBy`, `setVolume`, `setMuted`, `selectAudio`, `selectSubtitle`, `jump`, `remove`, `clear`, `progress(entryId)` |
| **`QueueModel`** | A `QAbstractListModel` over the queue's entry ids, with names from the audio and video engines (SQLite, UI thread, paged) |
| **`VideoItem`** | A `QQuickFramebufferObject` that draws hoardor's `VideoRenderer` on Qt's render thread. Without OpenGL (the software renderer in tests), QML shows a black placeholder instead |
| **Models** | `ItemModel` / `GroupModel` gain `progress` (0–1), `viewed`, and `liked` roles, read in one query per page (hoardor `states(entries)`) |

- **Connections and threads:**
  - **The player gets its own SQLite connection** for progress writes and for resolving files. hoardor uses that connection on its player thread only, following hoardor's one-connection-per-thread rule.
  - **The UI reads progress** on its own connection.

## 4. Edge cases

| Case | Expected |
|---|---|
| A track's drive is offline | the status line says "… is offline, skipped"; the queue goes on |
| A sleeping drive | "loading" in the bar (a moving segment on the seek line); the UI stays responsive |
| Every item in the queue fails | the player stops; one status line, not one per item |
| A copy picked on the page, then a better one found | what's queued doesn't change (entry ids) |
| The window closes while playing | progress saved; the player shut down before the bridge |
| No OpenGL (software renderer) | the video page shows black with its controls; audio plays |
| Full screen on a second monitor | the window's own screen; Esc returns |

## 5. Tests

- **`tst_player_bridge`** (new):
  - with `ao=null` / `vo=null`: play a generated album, prompt actions through the bridge
  - queued signals arrive on the UI thread
  - a missing file is reported once
  - progress roles in the models
- **The QML smoke test:**
  - the now-playing bar, the queue page, and the video page (with the software placeholder)
  - the prompt opens on a track click while something plays; screenshots
- **On the VNC display:** a real frame on the video page (a manual check; the CI runner has no GPU).

## 5a. As built (2026-10-03)

- **C++:**
  - `PlayerBridge` (`Player`): properties and invokables as §3, plus `ready`, `active`, `link` (where the title leads), and `revision`. `describe(entry)` names a track or a video. Several skipped files within 0.4 s make one status line.
  - `QueueModel`, with `count` and `remainingDuration`.
  - `VideoItem` (`VideoView`): a `QQuickFramebufferObject` drawing through hoardor's `VideoRenderer`. `ready` turns true once mpv's renderer exists.
  - `main.cpp` sets Qt Quick to OpenGL (P4), and creates the player bridge after the library bridge and before the QML engine.
  - **Tests:** `TYLI_PLAYER_OUTPUTS=null` (no sound, no frames) is set by `tests/CMakeLists.txt`. `noaudio` keeps the frames, for a check on a display.
- **QML:**
  - `PlayerBar`, `QueuePrompt`, `QueuePage`, `VideoPage`, `SeekLine`, and the new icons (`Icon.filled` for the liked heart)
  - `Main.view` decides the page (settings > search > video > queue > home > category); `playTracks` / `playVideo` / `openPlaying`
  - `CategoryView.play()` is how pages ask to play
  - the album, movie, and show pages, and `TrackList` (playing marker, hearts)
- **A video starts only once its picture can be drawn:** `Main.pendingVideo` waits for `VideoView.ready`. mpv would otherwise play it without a picture.
- **The video page stays drawn under a black cover while a video is loaded.** mpv waits up to 0.2 s for every undrawn frame, so a hidden picture would slow it. The page takes input only when it's the page shown.
- **Bindings that must follow likes and progress** read `Player.revision` inside the expression (`Player.revision >= 0 && …`). A bare `Player.revision;` statement can be compiled away, and then the bar's heart stopped following a like.
- **Checked on the VNC display (OpenGL, llvmpipe):** a 960×540 clip draws into the video page. A tiny 160×90 clip blown up 6× showed a grid of lines on llvmpipe: upscaling in software OpenGL, gone at a normal size. To check on a GPU in the Windows build (hoardor OI-P2).
- **Not built yet:**
  - markers on movie cards (a card is a group of copies; the movie page has the resume tooltip)
  - the artist page's play button
  - a space-bar toggle outside the video page
  - showing which copy plays on an album with several qualities

## 6. Open items

- **OI-U1:** the controls' sizes at 1.15 scale vs the mockups (drawn at 1.0).
- **OI-U2:** where the home page will show "continue watching" (later, with the home page).
