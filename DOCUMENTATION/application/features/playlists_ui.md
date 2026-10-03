# Playlists and liked songs: the UI

| | |
|---|---|
| Status | **Draft, waiting for the user's approval** (2026-10-03). Mockups: `playlist.html`, `liked.html`, `album-add.html`. The engine side: hoardor `features/playlists.md` |
| Branch | `abhinavp06/PLAYLISTS` |
| Scope | The user's choices: music only, manual playlists, and liked songs. Not now: smart playlists, M3U, the "Resume" section |

## 1. Screens

### 1.1 The sidebar
- Under the categories, a hairline and **"playlists"** with a **+**.
- **+** opens a name field right there (no popup). Enter creates the playlist and opens it; Esc cancels. An empty or taken name shows the reason in the status line.
- **Liked songs** first (heart icon), then the playlists a–z (playlist icon), each one line, elided.
- Clicking one opens its page in the content area, like the queue page (a top-level page; search, Settings, and the sections leave it as they do the others).
- Collapsed sidebar: icons only, with the name as a tooltip.

### 1.2 A playlist page (`playlist.html`)
- **Hero, like an album:**
  - **Cover:** a 2×2 mosaic of the first four different albums' covers; with fewer albums, one cover, or the playlist icon.
  - **Name:** "playlist", the name, and a pencil that renames in place (Enter keeps, Esc cancels).
  - **Facts:** "N tracks · length · updated <when>".
- **Actions** (top right): delete (a trash icon, then an inline "delete “Road Trip”? / delete / keep", no popup) and play.
- **Rows:** # · title · artist (link) · album (link) · length · heart · add to another playlist · × (remove, on hover).
  - **Click a row:** plays the playlist from that row (the queue prompt, as for albums).
  - **Reorder:** on hover the number becomes a handle; drag a row up or down, and a green line shows where it lands.
  - **Dimming:** an offline track is dimmed, and skipped when played.
- **Status line:** "added 11 tracks to Road Trip · 2 were already in it", "removed 1 track".

### 1.3 Liked songs (`liked.html`)
- The same layout: a heart cover, "liked songs", "N tracks · length", and play.
- **Rows:** a "liked" column (when), newest like first.
- **Not editable by hand:** the heart unlikes, and the row goes the next time the page opens. There's no rename, delete, or reorder.

### 1.4 Adding (`album-add.html`)
- **The playlist icon on a track row** (album page, playlist pages, liked songs, search tracks, the queue page, the now-playing bar) opens a small menu: "new playlist…", a line, then the playlists a–z, with a check on the ones it's already in (those can't be picked again).
- **The album page:** a playlist button left of the quality picker adds the whole album, in the chosen quality.
- **The queue page:** "save as playlist" next to "clear" makes a new playlist from the queue.
- **"New playlist…"** in a menu asks for the name inline in the menu, then adds.

## 2. Bridge

- **`Library` (the existing bridge)** gets the playlist calls, since they're library data:
  - `playlists` (a property: `[{id, name, tracks, durationMs, updatedMs}]`, a–z, with `playlistsChanged`)
  - `createPlaylist(name) → id`, `renamePlaylist(id, name)`, `deletePlaylist(id)`
  - `addToPlaylist(id, entryIds) → {added, skipped}`, `removeFromPlaylist(id, itemIds)`, `movePlaylistItem(id, itemId, index)`
  - `playlistEntries(id)`, `playlistsWith(entryId)`, `likedEntries()`
  - **Errors:** each call returns false (or -1) and puts its reason on the status line.
- **`PlaylistModel` (a new `QAbstractListModel`):** the rows of one playlist, or of liked songs (`playlistId` −1). Pages come from hoardor (`playlist_items` / `liked`), with each row's track looked up in the audio engine. It refreshes on `playlistsChanged` and on likes.
- **Playing:** `Player.playNow(entries, start)`, through the queue prompt.

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

## 5. Open items

- Sort order in the sidebar: a–z for now.
- Drag and drop from the album page straight onto a sidebar playlist: later, if the menus feel slow.
