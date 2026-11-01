# Unreleased

Work that is done but not yet part of a version. The newest entries come first.

### The Windows build: ffmpeg pinned to a build that stays online (2026-11-01, branch `abhinavp06/PLAYLISTS`)

**Summary:** The user's manual run of the Windows build failed at "Install ffmpeg" with `Invoke-WebRequest: Not Found`. The pinned ffmpeg zip (BtbN `autobuild-2026-10-01-13-06`) had been deleted: BtbN keeps its daily builds for about two weeks, and only the last build of each month for good. The run never got past the downloads. The cache step didn't help either, because the job's cache had also expired.

**What changed:** `FFMPEG_URL` and `FFMPEG_SHA256` now point at BtbN's month-end build `autobuild-2026-10-31-12-59`: `ffmpeg-n8.1.3-27-g8dca1586f4-win64-lgpl-shared-8.1.zip`, the same ffmpeg 8.1 branch with 13 more fixes. It has the same DLL names. Its SHA-256 (`5cef7bc4…56ab`) matches GitHub's digest and a download made on the VM. A comment in the workflow says to pin month-end builds only. The libmpv (shinchiro `20261003`) and Vulkan runtime (LunarG 1.4.363.0) downloads were checked and are still online.

**Decisions:**
- Rejected: BtbN's `latest` release. It moves every day, so a SHA-256 pin can't work with it.
- Rejected, for now: keeping a copy of the zip as a release asset in our own repository. That would be fully under our control, but it needs `gh` logged in on the VM and a release to hold it. Worth doing together with downloadable releases (PRODUCT I-43) if a month-end build ever disappears too.

**Tests:** the Windows run is the check.

**Files:** `.github/workflows/windows.yml`, `DOCUMENTATION/CHANGELOGS/unreleased.md`

### Full screen no longer switches the monitor's mode on Windows (2026-10-17, branch `abhinavp06/PLAYLISTS`; PRODUCT I-27)

**Summary:** The user, watching an episode in the Windows build with Windows' HDR on: entering or leaving full screen sometimes blanked the monitor for a second, and leaving it showed "HDR" in the monitor's corner, even for SDR episodes. A browser's full-screen YouTube doesn't do that.

**Cause:** TYLI never reads or changes HDR (no mpv option touches it, and with mpv's render API mpv doesn't own the window). The video page's full screen is Qt's `Window.FullScreen`: a borderless window covering the monitor, drawn with OpenGL (decision P4). GPU drivers on Windows take such a window over as *exclusive* full screen, which bypasses Windows' compositor and sets the display mode itself. With an 8-bit SDR surface that mode is SDR, so the monitor switched out of HDR going in and back into HDR coming out, blanking each time. The monitor's "HDR" message is it announcing the desktop's HDR mode again. Browsers stay composited, so they never switch.

**What changed:** on Windows, `setupApplication()` asks Qt to keep a one-pixel border on full-screen windows (`QWindowsApplication::setHasBorderInFullScreenDefault(true)`). With a border the window isn't "a borderless window covering the monitor", so it stays composited and the monitor keeps its mode. That's Qt's own documented workaround ("Qt for Windows - Specific Issues", full-screen OpenGL windows). It's a private Qt interface, so `tyli_ui` links `Qt6::GuiPrivate` on Windows only. Linux and macOS are unchanged.

**Decisions:**
- Rejected: drawing with Direct3D 11 instead of OpenGL (Qt's default on Windows). mpv's render API is OpenGL (P4); it would be a large change for this.
- Rejected: faking full screen with a frameless window one pixel larger or smaller than the screen. Fragile with display scaling and several monitors; Qt's flag does the same thing properly.
- Not changed: HDR videos are still tone-mapped to SDR by mpv, as before. Real HDR output (a 10-bit or scRGB surface) is a separate feature.

**Tests:** all 5 suites pass on Linux; the code is Windows-only, so the Windows build (CI) compiles it and the user's test is the real check: full screen in and out several times on the HDR monitor, an SDR and an HDR video. **Known limitation:** the border may show as a thin line at the screen's edges in full screen; if it's visible and bothers the user, we look again.

**Files:** `src/app_setup.cpp`, `CMakeLists.txt`, `DOCUMENTATION/PRODUCT.md` (I-27), `DOCUMENTATION/application/features/player_ui.md` §4, `DOCUMENTATION/application/ARCHITECTURE.md` (decision log)

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

