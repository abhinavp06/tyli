# Changelogs

The detailed history of TYLI. Each version has its own file, so no single file grows without bound. The rules are the same as hoardor's (`../hoardor/DOCUMENTATION/CHANGELOGS/README.md`).

| File | Contents |
|---|---|
| `unreleased.md` | Finished work that isn't in a version yet. New entries go here, newest first |
| `v<MAJOR>.<MINOR>.<PATCH>.md` | One file per version. Frozen once the version is cut |

## Versions

| Version | Date | Highlights |
|---|---|---|
| [Unreleased](unreleased.md) | — | Nothing yet |
| [v0.3.0](v0.3.0.md) | 2026-10-03 | **The player UI**: now-playing bar, queue prompt and page, video page, expanded player, likes, resume and viewed markers; **movies and shows like albums** (by director/genre, season cards and pages, the movie page with copies and more by its director and genres); posters from Plex or a frame; search from anywhere; lists that fill the width |
| [v0.2.0](v0.2.0.md) | 2026-10-03 | **Media library UI v1**: metadata-driven music, movie, and show pages with a quality picker for copies, thumbnails, live refresh while tags are read, search, browser-like back/forward that keeps your place, sizes for real screens, a scroll speed setting, preferences saved on Windows |
| [v0.1.0](v0.1.0.md) | 2026-10-02 | **File Sync UI + OLED theme**: Qt 6.12 app on hoardor `v0.1.0`, `Library` bridge, paged file lists, Settings → Library and General, OLED black + minimal green with its own controls and icons, live HTML mockups and VNC view, a Windows build on GitHub Actions |

## Versioning

- **SemVer:** `0.MINOR.PATCH` before 1.0. MINOR for a feature, PATCH for fixes, docs, or build changes.
- **The version** lives in `project(tyli VERSION ...)` in `CMakeLists.txt`. The first version, `v0.1.0`, is cut when `abhinavp06/FILE_SCANNER_INIT` merges, together with hoardor `v0.1.0`.
- **Cutting a version:** the same steps as hoardor:
  1. Move the `unreleased.md` entries, unchanged, into `v<version>.md`.
  2. Empty `unreleased.md`.
  3. Add a row to the table above.
  4. Check that the CMake version matches.
  5. Tag only when the user asks.

## Rules

- Never rewrite or delete a past entry. Corrections are new entries.
- A version file is frozen once it's cut.

## Entry template

Copy this into `unreleased.md` (newest first) and leave out any sections that don't apply.

```markdown
Entry template. Copy it under [Unreleased] and leave out any sections that don't apply.

### <Feature / change title> (YYYY-MM-DD, branch `abhinavp06/<topic>`, PR #<n>)

**Summary:** One or two sentences on what changed and why it matters.

**Added**
- ...

**Changed**
- ...

**Fixed**
- ...

**Removed**
- ...

**Design decisions and trade-offs**
- The decision, the alternatives considered, and why this one won.

**Files**
- `path/to/file`: what changed in it.

**Tests**
- What is covered, the edge cases included, and how to run them.

**Performance**
- Benchmark results against the targets, if relevant.

**Known limitations / follow-ups**
- ...
```
