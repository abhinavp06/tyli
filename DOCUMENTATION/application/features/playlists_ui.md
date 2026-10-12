# Playlists and liked songs: the UI

| | |
|---|---|
| Status | **Shipped in `v0.4.0`** (2026-10-12; built 2026-10-03, §6), waiting for the user's Windows test. Approved with changes (§0). Mockups: `playlist.html`, `liked.html`. The engine side: hoardor `features/playlists.md` |
| Branch | `abhinavp06/PLAYLISTS` |
| Scope | The user's choices: music only, manual playlists, and liked songs. Not now: smart playlists, M3U, the "Resume" section |

## 0. The user's review (2026-10-03)

1. "Okay with almost everything."
2. **No "add the album to a playlist"** button: removed (and `album-add.html`).
3. **Rename:** the field has a green ✓ and a × inside it, like the search field's ×. Enter and Esc do the same.
4. **The sidebar's order:**
   - Liked Songs always on top.
   - Then pinned playlists, then the rest, each in the user's own order: drag to reorder within a group.
   - The **pin** sits on the right: it shows on hover, and stays on pinned ones.
5. **"Liked Songs"** is capitalized: the one name in the UI's labels that isn't lowercase, because it's a name, not a label.
6. **A green speaker on the right** of a playlist (and of Liked Songs) while what's playing was started from it. A song that merely is in a playlist doesn't light it.
7. The design language: actions on the right of their row (pin, speaker), green for what's active.

## 1. Screens

### 1.1 The sidebar
- Under the categories, a hairline and **"playlists"** with a **+**.
- **+** opens a name field right there (no popup). Enter creates the playlist and opens it; Esc cancels. An empty or taken name shows the reason in the status line.
- **Liked Songs** first (heart icon), fixed. Then the pinned playlists, a faint line, and the rest (playlist icon), each in the user's order and each one line, elided.
- **On the right of a row:** a green speaker while playing from it, then the pin (on hover; always on pinned ones).
- **Reordering:** drag a playlist up or down within its group; a green line shows where it lands.
- Clicking one opens its page in the content area, like the queue page (a top-level page; search, Settings, and the sections leave it as they do the others).
- Collapsed sidebar: icons only, with the name as a tooltip.

### 1.2 A playlist page (`playlist.html`)
- **Hero, like an album:**
  - **Cover:** a 2×2 mosaic of the first four different albums' covers; with fewer albums, one cover, or the playlist icon.
  - **Name:** "playlist", the name, and a pencil that renames in place: a field with ✓ and × inside (Enter and Esc too).
  - **Facts:** "N tracks · length · updated <when>".
- **Actions** (top right): delete (a trash icon, then an inline "delete “Road Trip”? / delete / keep", no popup) and play.
- **Rows:** # · title · artist (link) · album (link) · length · heart · add to another playlist · × (remove, on hover).
  - **Click a row:** plays the playlist from that row (the queue prompt, as for albums).
  - **Reorder:** on hover the number becomes a handle; drag a row up or down, and a green line shows where it lands.
  - **Dimming:** an offline track is dimmed, and skipped when played.
- **Status line:** "added 11 tracks to Road Trip · 2 were already in it", "removed 1 track".

### 1.3 Liked Songs (`liked.html`)
- The same layout: a heart cover, "Liked Songs", "N tracks · length", and play.
- **Rows:** a "liked" column (when), newest like first.
- **Not editable by hand:** the heart unlikes, and the row goes the next time the page opens. There's no rename, delete, or reorder.

### 1.4 Adding (`album-add.html`)
- **The playlist icon on a track row** (album page, playlist pages, liked songs, search tracks, the queue page, the now-playing bar) opens a small menu: "new playlist…", a line, then the playlists a–z, with a check on the ones it's already in (those can't be picked again).
- **The queue page:** "save as playlist" next to "clear" makes a new playlist from the queue.
- **"New playlist…"** in a menu asks for the name inline in the menu, then adds.

## 2. Bridge

- **`Player` (the player bridge)** gets the playlist calls. As built: it already holds hoardor's `player::Library` (likes) on its own connection, and playlists are that engine's:
  - `playlists` (a property: `[{id, name, pinned, tracks, durationMs, updatedMs}]`, in the sidebar's order, with `playlistsChanged`)
  - `movePlaylist(id, index)`, `setPlaylistPinned(id, pinned)`
  - `createPlaylist(name) → id`, `renamePlaylist(id, name)`, `deletePlaylist(id)`
  - `addToPlaylist(id, entryIds) → {added, skipped}`, `removeFromPlaylist(id, itemIds)`, `movePlaylistItem(id, itemId, index)`
  - `playlistInfo(id)`, `playlistEntries(id)`, `playlistsWith(entryId)`, `likedEntries()`, `likedInfo()`
  - **Errors:** each call returns false (or -1) and puts its reason on the status line.
- **`PlaylistModel` (a new `QAbstractListModel`):** the rows of one playlist, or of liked songs (`playlistId` −1). Pages come from hoardor (`playlist_items` / `liked`), with each row's track looked up in the audio engine. It refreshes on `playlistsChanged` and on likes.
- **Playing:** `Player.playNow(entries, start)`, through the queue prompt.
- **What's playing from:** `Player.source`, set by `Player.playNow(entries, start, source)` (`{kind: "playlist", id}` or `{kind: "liked"}`, else empty) is set when playback starts from a playlist page, and cleared when anything else replaces the queue or the queue is cleared. The sidebar's speaker follows it.

## 3. Edge cases

- Renaming to the same name in another case is allowed; to a taken one, refused with the reason.
- Deleting the playlist that's playing: playback goes on, since the queue holds files, not the playlist.
- A playlist of files that left the library: those rows are gone; one that's empty shows "no tracks yet: add some with the playlist icon on any track".
- Dragging past the top or bottom: to the first or last place.
- Liked songs with no likes: "nothing liked yet: the heart on any track".
- Very long playlists: the model pages; reordering works on visible rows (drag scrolls the list).

## 4. Tests

- **`tst_library_bridge`:** create, rename, delete, add (skips), remove, move, errors.
- **`tst_media_models`:** `PlaylistModel` paging, order after a move, liked songs order.
- **`tst_qml_smoke`:**
  - the sidebar section and the new-playlist field
  - a playlist page and liked songs (screenshots)
  - the add menu on a track and on an album
  - delete's inline confirm
  - reordering by drag

## 6. As built (2026-10-03)

- **`Sidebar`:**
  - The categories (a `Repeater` now: a `ListView` sized to its own content created no rows) and the playlists section, in one `Flickable`.
  - **+** opens a `NameField`.
  - Liked Songs, then the pinned and the other playlists, each group a `Column` with its own drop line.
  - Press and drag a row to reorder it within its group; a click opens it.
  - The speaker shows while `Player.source` is that playlist; the pin shows on hover and stays on pinned ones.
- **`PlaylistPage`** (a top-level page, `Main.view` "playlist", the queue page's margins):
  - the hero (a mosaic of the first four albums' covers via `PlaylistModel.coverEntries`; one cover; or the playlist icon; a heart for Liked Songs)
  - rename in a `NameField`, delete with an inline confirm, play
  - rows: drag by the handle that replaces the number on hover; artist and album links; the heart; add to another playlist; × on hover
  - **Liked Songs:** a "liked" column and no editing; its rows go when unliked
- **`NameField`** (new): a `TyField` with ✓ and × inside (inset like the search ×), `confirmed(name)` and `cancelled()`. Used by the sidebar, the rename, the add menu, and the queue page.
- **`PlaylistMenu`** (new): a `Popup` under the clicked icon, one instance in `Main` (`Window.window.addToPlaylist(ids, anchor, label)`).
  - It offers "new playlist…" (a `NameField` in place) and the playlists, with checks.
  - Its rows are a `Column` in a `Flickable`, since a `ListView` sized to its content made none.
- **Where tracks are added:**
  - an album's track rows (`TrackList`: the playlist icon is live)
  - a playlist's or Liked Songs' rows
  - the now-playing bar (songs only)
  - the queue page's "save as playlist" (a `NameField`; then `Player.queueIds()`)
- **The queue prompt** carries the source, so "clear queue and play" from a playlist lights its speaker; "add to queue" doesn't.

## 5. Open items

- Drag and drop from the album page straight onto a sidebar playlist: later, if the menus feel slow.
