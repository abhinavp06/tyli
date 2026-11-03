# Unreleased

Work that is done but not yet part of a version. The newest entries come first.

### Full screen's border painted black on Windows; a track from search flashes on its album (2026-11-03, branch `abhinavp06/PLAYLISTS`; PRODUCT I-27, I-54)

**Summary:** two things the user found in the latest Windows build:
1. **A white border around the whole window in full screen** (the video page).
2. **A track in the search results opens its album, but the track is hard to find there.** The user: "highlight it, so it's easier for me to find; maybe a green flash surrounding it".

**1. The border (`src/app_setup.cpp`, Windows only):**
- **Cause:** the one-pixel full-screen border from the HDR fix (2026-10-17, below). Qt adds it as a classic `WS_BORDER` frame, and Windows draws that in its light theme's frame color: white. The border is the fix itself (the video surface no longer covers the whole monitor, so the GPU driver doesn't take it over as exclusive full screen), so it stays, invisible.
- **What changed:** a native event filter (`BlackFullScreenBorder`) recognizes Qt's full screen with a border (a `WS_POPUP` window with `WS_BORDER` and no caption, covering its monitor) and, for that window only:
  - paints the frame black on `WM_NCPAINT`, and again after `WM_NCACTIVATE` (Windows redraws the frame on focus changes without a `WM_NCPAINT`);
  - on entering full screen (`WM_WINDOWPOSCHANGED`), turns off Windows 11's own accent outline (`DWMWA_BORDER_COLOR` = none) and DWM's frame drawing (`DWMWA_NCRENDERING_POLICY` = disabled), and puts both back on leaving it. On Windows 10 these calls fail harmlessly.
  - Windowed mode is untouched: its frame has a caption, so the filter ignores it.
- `tyli_ui` links `dwmapi` on Windows (`CMakeLists.txt`).
- **Decisions:**
  - Rejected: dropping the border. The monitor would blank and leave HDR again (I-27).
  - Rejected: a window one pixel larger than the screen, with the border off screen. Same reason as on 2026-10-17: fragile with display scaling and several monitors.
  - Rejected: `QStyleHints::setColorScheme(Dark)`, which darkens Windows' frames for the whole app. Its frame is dark grey, not black, and it changes Qt's palette on Windows as a side effect.
- **Tests:** Windows-only code, so CI compiles it and the user's test is the real check: full screen in and out, alt-tab and a click on another monitor while in full screen (focus changes), no white line, and still no blanking with HDR on. **If a line still shows**, the user says its color (white, grey, or the accent color), which tells which part of Windows draws it.

**2. The track from search (`qml/SearchView.qml`, `qml/AlbumView.qml`, `qml/TrackList.qml`):**
- A track result passes `highlightEntry` and `highlightTitle` with the album it opens. The album page gives them to its `TrackList` (`highlight`).
- The list scrolls the track into view (only if it isn't already) and flashes its row: a green outline (`Theme.accent`) over a faint green fill (`Theme.selection`), drawn under the row's text. It pulses twice and fades, about 2 s in all, once per page.
- The row is the same file when the copy shown has it; else the track with the same title (the album opens on the remembered or best quality, which may be another copy of it). No match, no flash.
- **Decisions:** the flash fades rather than staying, so the page looks normal again; the playing track keeps its own green marker. Rejected: switching the album to the copy the result came from. The quality the user picked for the album stays.
- **Tests:** `tst_qml_smoke::searchAcrossTheLibrary` opens a track result, checks the flashed row's title, takes a screenshot mid-flash (`search-track-flash.png`, checked by eye: outline and fill under the text), waits for it to fade, and goes back to the results. All 5 suites pass.
- **Not done:** other ways into an album (the player bar's album link, the expanded player) don't flash yet; the playing track is already green there. Easy to add if the user wants it.

**Files:** `src/app_setup.cpp`, `CMakeLists.txt`, `qml/SearchView.qml`, `qml/AlbumView.qml`, `qml/TrackList.qml`, `tests/tst_qml_smoke.cpp`, `DOCUMENTATION/application/features/media_listing_ui.md` §7c, `DOCUMENTATION/application/features/player_ui.md` §4, `DOCUMENTATION/PRODUCT.md` (I-27, I-54), `DOCUMENTATION/CHANGELOGS/unreleased.md`

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

