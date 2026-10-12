# TYLI: product design and ideas

This is the product's home page: the vision, the principles every design follows, the release plan, and **every idea the user has had**, each with its status and links to where it's designed, built, or tracked. Technical detail stays in the feature, engine, and changelog docs. This file only points to them.

**How it's kept (the user's request, 2026-10-04):**
- The user brings ideas in any shape. Claude files each one here under its area, with an ID (`I-<n>`, never reused), a status, and links. If an idea changes an existing one, the existing row is updated rather than duplicated.
- When an idea is designed, built, or shipped, its row gets the link: a feature doc (§ number), a changelog entry (version file + heading), a TODO in the workspace checkpoint, or a decision-log row.
- When the user decides against an idea, it moves to §6 with the reason. Nothing is deleted.
- Links into hoardor (`../../hoardor/...`) work in the workspace layout (`tyli/` next to `hoardor/`), not on GitHub.

**Status words:**

| Status | Meaning |
|---|---|
| **Shipped** | In a cut version (named) |
| **Built** | Done on a branch, not in a version yet |
| **Next** | On the current TODO list (workspace `CLAUDE.md`, "NEXT SESSION") |
| **Planned** | Agreed to do, not scheduled |
| **Idea** | Recorded, not decided yet |
| **Parked** | Decided to wait, with the condition for picking it up |
| **Rejected** | See §6 |

---

## 1. Vision

One snappy, low-memory, **fully offline** home for the user's whole media and writing library (music, movies, TV shows, and later books, blogs, and notes), replacing the several apps they use today (Plex among them).

- **The library lives on external HDDs today** (about 1 TB of music, about 5 TB of other media), and later on RAID, a NAS, or a home server. Every design must work on all of them without code changes.
- **TYLI is the app; hoardor is the engine.** hoardor stays headless, so it can later run on a home server, for example for local AI analysis of the user's own data.
- **A portable library** (the user, 2026-10-04): everything TYLI knows lives in one database (or one export file) that can be imported on another machine and just works when the media is in the same place. Folder locations can be changed in Settings.

## 2. Product principles

Each one names where it's recorded, so a design can be checked against it.

| # | Principle | Recorded in |
|---|---|---|
| P1 | **Offline only.** Metadata comes from tags, files next to the media, Plex's local database (read-only), or file names. No online lookups. | hoardor `ARCHITECTURE.md` decision log (2026-10-02, 2026-10-03) |
| P2 | **Nothing offloaded to the media folders** (the user, 2026-10-04). hoardor reads what's already next to the media (posters, `.nfo`, subtitles) and never writes there. The one exception is the `.hoardor-root` marker, kept for now (I-3). | hoardor `ARCHITECTURE.md` §6; I-1, I-3 |
| P3 | **The UI never freezes and never wakes a sleeping drive** just to show something. Browsing reads SQLite only. | `application/ARCHITECTURE.md` §2; hoardor `ARCHITECTURE.md` §6 |
| P4 | **Memory stays flat** with library size: lists page from SQLite. | hoardor `ARCHITECTURE.md` §4 |
| P5 | **Configurable by design:** tunables are settings, and every hoardor setting is editable in TYLI. Prune what nobody touches later. | hoardor `ARCHITECTURE.md` §3a |
| P6 | **The look:** OLED black, minimal green, our own icons, no popups except the queue prompt. | `application/features/theme.md` |
| P7 | **Mockup first:** every UI idea is an HTML mockup the user reviews before QML is written. | `mockups/`, `application/ARCHITECTURE.md` §4 |
| P8 | **Copies of the same thing are one item with a quality picker** (best by default, the user's choice remembered). | `application/features/media_listing_ui.md` §3 |
| P9 | **Every product decision's performance cost is written down** (the performance map, I-40). | I-40 |

## 3. Release plan

| Version | What | Where |
|---|---|---|
| `v0.1.0` | File Sync (folders, categories, background sync), the OLED theme, the Windows zip | `CHANGELOGS/v0.1.0.md`, hoardor `CHANGELOGS/v0.1.0.md` |
| `v0.2.0` | Media library v1: tags with ffmpeg, generic queries, every view, search, copies and quality | `CHANGELOGS/v0.2.0.md` |
| `v0.3.0` | The player (libmpv): bar, queue, video page, expanded player; posters from Plex or a frame; director/genre sections, season and movie pages | `CHANGELOGS/v0.3.0.md` |
| `v0.4.0` (2026-10-12) | Playlists and Liked Songs, show episodes as a queue, the missing shows (specials, extras out of the grids) | `CHANGELOGS/v0.4.0.md`, hoardor `CHANGELOGS/v0.4.0.md` |
| `v0.5.0` (next) | English subtitles by default (I-20), long-playlist dragging (I-31), the Extras section (I-18), and the fixes from the user's Windows test | workspace TODO |
| `v0.6.0`+ | The metadata work (I-10 to I-14), then whatever the v1/v2 triage picks | workspace TODO E–F |
| **`v1.0.0`** | **Gate (the user, 2026-10-03):** text support (books, blogs; I-60) and "another thing" the user will name (I-99). Versions go on normally until then. | — |

**The v1/v2 triage** (the user, 2026-10-04): after the metadata work, the user goes through every Idea and Planned row below and marks it v1 or v2.

---

## 4. Ideas by area

### 4.1 Library, files, and portability

| ID | Idea | Status | Links |
|---|---|---|---|
| I-1 | **Portable library:** one database or custom export file, imported on another machine, works when the media is in the same place; folders re-pointed in Settings. Today's data is `library.db` + `preferences.ini` + `thumbnails/` (regenerable) in AppData, and paths are stored relative to each folder, so the design fits. | Idea (v1/v2 triage) | §1; `application/ARCHITECTURE.md` §3; hoardor `engines/file.md` (storage model) |
| I-2 | **Moving and renaming files from the app** (the user, 2026-10-04: "we'll get to a point"). Breaks P2 on purpose; needs an architecture decision when it comes. Triggers I-14. | Idea | hoardor `ARCHITECTURE.md` §6 ("hoardor never writes to media storage") |
| I-3 | **Remove the `.hoardor-root` marker** if root identity can work without it (volume ID + a sample of files). The user: keep it for now, "later we'll see if we can remove it". | Parked | hoardor `engines/file.md` (root identity); `features/file_sync.md` |
| I-4 | Moves and renames detected (partial fingerprint), keeping play counts and metadata. | Planned | hoardor `engines/file.md` §4.1 |
| I-5 | Sync when a drive is plugged in; per-volume support on each OS; macOS. | Planned | hoardor `engines/file.md` §4 (phase 4) |
| I-6 | A sync yields to playback on the same HDD (OI-1). Check for stutter during daily use first. | Parked (needs a measurement) | hoardor `features/file_sync.md` §6; `engines/player.md` §4 |

### 4.2 Metadata, grouping, and editing

| ID | Idea | Status | Links |
|---|---|---|---|
| I-10 | **The folder rule.** An album is the folder that holds the audio files. A show is its top folder and a season is its season folder: the user's layout is `SHOWS/<show>/<season>/<episode>` (the user, 2026-10-04: "ideally, yes"). hoardor gets a generic Folder field; TYLI gets the rule. Should also bring the album count closer to Plex's (I-11). | Next (TODO E) | `application/features/media_listing_ui.md` §8; workspace TODO E |
| I-11 | Album count vs Plex: about 4,557 vs about 1,834. Expected to improve with I-10. **Checked 2026-10-12 against the user's `albums_list.txt`:** 26,997 audio files sit in 1,917 folders, and about 1,852 once `CD1`/`Disc 2` folders join their parent. That's within about 1% of Plex, which confirms the folder rule for music. Box sets whose discs are separate albums (e.g. `…/STILL BEYOND CD01 <title>`) must stay apart: only folders named just like a disc (`CD1`, `Disc 2`, `(CD1)`) merge. | Next (with I-10) | `application/features/media_listing_ui.md` §8 |
| I-12 | **Movie merge strategies** (the user, 2026-10-04: "come up with merge strategies as much as possible"). When are two files the same movie, and when are they two movies? See §5.1. **Checked 2026-10-12 against `movies_list.txt`:** 520 videos are 199 movies and 321 extras (featurettes, trailers, menu art, samples). Extras show up as movies, and identical extra names merge across films ("Trailer" becomes one movie with 15 copies). A `Sample/` file merges into its film as a copy. Title + year merges every real duplicate correctly (9 films in two copies), and only 2 films lack a year (one has it in its folder name only). | Next (TODO E, design) | §5.1 |
| I-13 | **Season posters, read-only:** an image already in the season folder, then Plex's season poster, then the show's poster (today). | Next (with I-10) | hoardor `features/posters.md`; `application/features/media_listing_ui.md` §7g |
| I-14 | **Metadata editing in the app.** Proposed: edits stored in the database as an override layer, never written into tags or folders (P2, I-1). **Reminder (the user, 2026-10-04):** when editing metadata and moving files from the app (I-2) arrive, **revisit the whole metadata, tagging, and grouping logic** (I-10, I-12, copy grouping). | Idea (design with the user) | §5.2 |
| I-15 | Re-read a video when a poster or `.nfo` is added next to it later. | Planned | hoardor `CHANGELOGS/v0.3.0.md` (posters entry, follow-ups) |
| I-17 | **Shows missing from the Shows grid** (the user, 2026-10-12; a bug). A video counts as an episode only from its `.nfo`, its tags, or a name matching `SxxEyy`/`1x02` (or a number in an exact `Season N` folder); everything else becomes a movie, and a Shows category lists only episodes. Checked against the user's real list (1,773 videos, `shows_list.txt`): 806 become movies. Lost entirely: Evangelion (`[Group] Show - 01`), Dragon Ball (`Episode 1 Title`, no season folder), The Bads of Bollywood (`Show E01`). Also lost: 499 featurettes, DBZ's movies, OVAs, and specials, and Evangelion's films. Shows that do appear are often named after the release folder (`Breaking Bad (2008) Season 1-5 S01-S05 (1080p BluRay …)`). The user's `DBZ/` folder holds two shows, so "the show is the top folder" (I-10) doesn't hold as stated. | Shipped (`v0.4.0`, 2026-10-12): the user chose extras on their show's or movie's page, films/OVAs in a show's folder as its Specials, samples skipped | hoardor `features/shows_and_extras.md`; `application/features/media_listing_ui.md` §7h; both `CHANGELOGS/` |
| I-18 | **An Extras section on show and movie pages** (the user, 2026-10-12): featurettes, deleted scenes, trailers, … listed on the page of what they belong to, grouped by season for shows. hoardor already stores them (type 3) and ties each to its show or movie. | Next (mockup first) | `application/features/media_listing_ui.md` §7h; hoardor `features/shows_and_extras.md` §6 |
| I-16 | Movie collections and franchises. | Rejected for now | `application/features/media_listing_ui.md` §8 |

### 4.3 Player

| ID | Idea | Status | Links |
|---|---|---|---|
| I-20 | **Subtitles:** on by default, English by default, chosen automatically (the user changes it if needed), with a switch and a languages field in Settings. No subtitle work during sync. | Next (TODO A1) | `application/features/player_ui.md` §5a2; hoardor `features/player.md` |
| I-21 | **"Resume" in the sidebar:** one player or two (music and video apart, or merged deliberately), with a way back to the music or the show you were on. | Idea (v1/v2 triage) | `application/features/player_ui.md` §6 (OI-U3) |
| I-22 | Smaller items: markers on movie cards, the artist page's play button, the space bar toggling play/pause everywhere, a larger cached cover for the expanded player. Waiting for the user's UI/UX enhancements list. | Parked | workspace TODO F |
| I-23 | Bit-perfect output (exclusive WASAPI), ReplayGain, chapters, a saved queue. | Planned | hoardor `engines/player.md` §4 |
| I-24 | Skip Intro (finding the audio that repeats across episodes). | Planned | hoardor `engines/player.md` §4 |
| I-25 | A visualizer. | Idea | hoardor `ARCHITECTURE.md` §2 (ring buffer candidate) |
| I-26 | Video grain vs Plex. | Shipped (solved, the user 2026-10-04) | `CHANGELOGS/v0.3.0.md` ("Video grain checked") |

### 4.4 Playlists

| ID | Idea | Status | Links |
|---|---|---|---|
| I-30 | Manual playlists and Liked Songs: pinned first, the user's own order, a speaker for "playing from here". | Shipped (`v0.4.0`) | `application/features/playlists_ui.md`; hoardor `features/playlists.md`; `CHANGELOGS/unreleased.md` |
| I-31 | **Long playlists:** auto-scroll while dragging, so a song can move anywhere (today only within the 200 loaded rows). | Next (TODO A2) | `application/features/playlists_ui.md` |
| I-32 | Drag from an album straight onto a sidebar playlist. | Idea (if the menus feel slow) | `application/features/playlists_ui.md` §5 |
| I-33 | Smart playlists, M3U import/export. | Rejected for now (the user: music only, manual) | `application/features/playlists_ui.md` |

### 4.5 Quality, performance, and process

| ID | Idea | Status | Links |
|---|---|---|---|
| I-40 | **The performance map** (the user, 2026-10-04): which product decision costs which feature how much. Every new design fills in its rows. | Next (TODO D: propose place and format) | workspace TODO D |
| I-41 | Several workers per SSD (parallelism beyond one per drive). | Planned | hoardor `ARCHITECTURE.md` decision log (2026-10-02) |
| I-42 | CI on all three OSes (today: a Windows build only). | Parked | hoardor `ARCHITECTURE.md` decision log (2026-10-01) |

### 4.6 Home, discovery, and intelligence

| ID | Idea | Status | Links |
|---|---|---|---|
| I-50 | **The home page** (`tyli_`), blank today: recommendations and "continue watching" live there. | Planned (after the player and playlists) | `application/ARCHITECTURE.md` decision log (2026-10-03); `player_ui.md` §6 (OI-U2) |
| I-51 | Recommendations (hoardor's engine work). | Idea | hoardor `CLAUDE.md` (TYLI's role) |
| I-52 | Local AI analysis of the user's own data on a home server, with hoardor as a headless daemon. | Idea (long term) | §1; hoardor `ARCHITECTURE.md` §1 |
| I-53 | **Speech-to-text** (the user, 2026-10-06; use cases to be explained later): store the audio **and** its transcript. Confirmed speech-to-text, not text-to-speech, with Cactus Compute's **Whistle** as the candidate model. The user's storage idea: hoardor keeps only the locations, and the files live in the app's data folder. See §5.3. | Idea (feasibility checked 2026-10-06) | §5.3 |

### 4.7 Text

| ID | Idea | Status | Links |
|---|---|---|---|
| I-60 | **Text support: books, blogs, notes**, with a reader. A separate big build. Part of the `v1.0.0` gate. | Planned (the v1 gate) | hoardor `ARCHITECTURE.md` decision log (2026-10-03, no Books by default) |

### 4.8 Unnamed

| ID | Idea | Status | Links |
|---|---|---|---|
| I-99 | "Another thing" for `v1.0.0`, to be named by the user. | Waiting for the user | §3 |

---

## 5. Idea details

### 5.1 Movie merge strategies (I-12; to expand when TODO E is designed)

Today, copies of a movie group when hoardor's group key matches (title + year from tags, `.nfo`, or the name), and TYLI turns the group into a quality picker (`src/copy_grouper.hpp`). Candidate strategies, strongest signal first:

1. **An external ID** in a `.nfo` (IMDb, TMDb) or Plex's database (its GUIDs, read-only): the same ID means the same movie, whatever the title says.
2. **Title + year, normalized:** case, punctuation, a leading "The", diacritics, Roman numerals vs digits, "&" vs "and".
3. **The folder:** a folder with one movie is that movie. In a shared folder, title + year decides. Copies in separate folders merge when 1 or 2 matches.
4. **Editions:** "Director's Cut", "Extended", "Theatrical" (from the name, the `.nfo`, or Plex). Either separate movies or labelled copies. **The user's call.**
5. **Duration as a tie-breaker:** same title and year but a very different length is probably a different cut or a different movie.
6. **A manual merge or split, stored in the database** (I-14), which wins over every rule.

The same questions apply to albums (MusicBrainz IDs in tags, the folder rule) and to shows (the top folder vs the show tag).

### 5.2 Metadata editing and moving files (I-14, I-2)

- **Editing (proposed):** an override layer in the database. Tags and folders on disk stay untouched (P2), and the edits travel with the portable library (I-1).
- **Moving files from the app (later):** the first time hoardor would write to media storage beyond the marker. It needs an architecture decision with the user (hoardor `ARCHITECTURE.md` §6), and it needs I-4 (moves detected) so nothing is lost.
- **When either arrives:** revisit the whole metadata, tagging, and grouping logic together (the user's reminder, 2026-10-04).

---

### 5.3 Speech: Whistle (I-53; checked 2026-10-06, not designed)

- **What Whistle is:** a speech-to-text model (released 2026-10-02, Apache 2.0, one 16.9 MB `.cact` file, CPU only).
  - It transcribes 16 kHz mono audio, up to 30 s per pass. Longer audio is cut into chunks (Cactus also has a streaming call).
  - It gives word timestamps and speech embeddings.
  - 7 languages: English, German, French, Spanish, Italian, Dutch, Polish.
  - It runs on Cactus's Needle engine (C++, a C API in `needle.h`: `needle_load`, `needle_transcribe`, `needle_embed`).
  - Links: [Hugging Face](https://huggingface.co/Cactus-Compute/whistle), [Needle](https://github.com/cactus-compute/needle).
- **The direction:** speech-to-text, which is what Whistle does (the user confirmed 2026-10-06; text-to-speech isn't wanted).
- **Where it would live: hoardor.** It's domain work with a third-party library (ARCHITECTURE: business logic in hoardor), run in the background by `master` like tag reading. TYLI only shows the results.
  - hoardor already decodes and resamples audio with ffmpeg, so it can feed Whistle 16 kHz mono chunks.
  - Not decided: a new engine (e.g. `speech`) or part of `audio`/`text`.
- **Storage (the user's idea, fits P2 and I-1):** hoardor stores locations and the caller picks the folder (like `thumbnails/`), so hoardor never names the app.
  - Suggestion to discuss: the transcript and word timestamps go in SQLite (small, searchable with the existing FTS5 search, and portable in the one database file).
  - Generated audio goes in files in the app's data folder, stored relative to it.
- **Risks before committing:**
  - **Windows:** prebuilt engines are listed for macOS arm64, Linux x86-64, and WebAssembly. Windows x64 (MSVC) isn't listed, and it's the user's main OS. Check that the engine builds with MSVC, or ask Cactus.
  - **Maturity:** the model is days old, and its C API is barely documented. x86 speed is unmeasured (the published numbers are from Apple M4).
  - **Threading:** "one model per process, not thread-safe", which is fine on one hoardor worker.
- **First step when picked up:** a playground experiment (`playground/`, like `hoardor_read`) on the Linux VM: load Whistle, transcribe a few clips, measure speed and memory on x86. Then a Windows build check in the workflow. Discuss the dependency with the user before it goes into hoardor proper.

## 6. Decided against (with the reason)

| Idea | Why not | Recorded in |
|---|---|---|
| TYLI as a local web server with a browser UI | Browsers limit playback formats and bit-perfect output unless the server transcodes. hoardor stays headless, so a server is still possible later. | `application/ARCHITECTURE.md` decision log (2026-10-02) |
| Qt Multimedia, or our own ffmpeg + miniaudio player | libmpv plays everything, gapless, with subtitles and HDR. | hoardor `ARCHITECTURE.md` decision log (2026-10-03) |
| TagLib as a second tag reader | ffmpeg already reads tags. | hoardor `ARCHITECTURE.md` decision log (2026-10-02) |
| Realtime folder watching | On-demand Sync is the truth; watching may come back only as a hint that starts a sync. | hoardor `ARCHITECTURE.md` §6 |
| Listing media by file and folder names only | Names must come from metadata. | hoardor `ARCHITECTURE.md` decision log (2026-10-02) |
| A Books category by default | Not until text has its own build (I-60). | hoardor `ARCHITECTURE.md` decision log (2026-10-03) |
| Chasing the suspected 27-second mtime shift on Windows | It only affects the "still being copied" check, and the next sync repairs it. Revisit if something odd shows up. | workspace checkpoint (2026-10-04) |

## 7. Where things live

| What | Where |
|---|---|
| The current TODO and checkpoint | workspace `CLAUDE.md` ("NEXT SESSION"), not version-controlled |
| How TYLI is built, and its decisions | `application/ARCHITECTURE.md` |
| How hoardor is built, and its decisions | `../../hoardor/DOCUMENTATION/application/ARCHITECTURE.md` |
| Feature plans | `application/features/` (UI), `../../hoardor/DOCUMENTATION/application/features/` (engine) |
| Engine references | `../../hoardor/DOCUMENTATION/application/engines/` |
| Every table and migration | `../../hoardor/DOCUMENTATION/application/DATABASE.md` |
| Every file, type, and test | `../../hoardor/DOCUMENTATION/application/CODE_TREE.md` |
| What changed, version by version | `CHANGELOGS/`, `../../hoardor/DOCUMENTATION/CHANGELOGS/` |
| Mockups | `mockups/` (served by `tools/mockup_server.sh`) |
