# Unreleased

Work that is done but not yet part of a version. The newest entries come first.

### The Windows build: only the newest zip is kept; a smoke-test race fixed (2026-10-12, branch `abhinavp06/PLAYLISTS`)

**Summary:** The user's first run after `v0.4.0` failed twice (log shared by the user):
- `tst_qml_smoke::playlistsEndToEnd` failed on the Windows runner (it passes on Linux).
- The zip couldn't upload: "Artifact storage quota has been hit".

**What changed:**
- **The test:** the playlist page shows Liked Songs (no trash button), then "Road" in the same page, so its actions row lays the trash out again on the next frame. The test clicked at once, at the button's old position, so the click missed on the slower runner and the inline confirm never appeared. It now waits for the trash to show plus 100 ms before clicking, and the same before clicking the confirm, which shifts the row again. That's the pattern the rename step already used.
- **The workflow:** the repository is private, and a private repository's artifact storage is small (500 MB on the free plan). With a 14-day retention, a few pushes filled it.
  - A new step, "Delete older zips", deletes the earlier `tyli-windows` artifacts with the job's own token, before each upload. The job now has `permissions: actions: write, contents: read`.
  - The zip is kept 3 days.
  - GitHub recounts the storage every 6 to 12 hours, so an upload can still fail once right after the cleanup.

**Decisions:**
- Rejected: reducing the zip's contents. Every file in it is needed to run.
- Rejected: uploading only on some pushes. The user tests most pushes.

**Tests:** `tst_qml_smoke` passes on Linux (all 5 suites). The Windows run is the real check.

**Files:** `tests/tst_qml_smoke.cpp`, `.github/workflows/windows.yml`, `CLAUDE.md`

