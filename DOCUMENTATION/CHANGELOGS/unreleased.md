# Unreleased

Work that is done but not yet part of a version. The newest entries come first.

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

