# Unreleased

Work that is done but not yet part of a version. The newest entries come first.

### Settings → About names the author (2026-10-03, branch `abhinavp06/PLAYLISTS`)

**Summary:** The user asked for their name in About: "tyli x.x.x · by abhinavp06". The About line now reads "tyli 0.3.0 · by abhinavp06" (the version is the app's, from CMake).

**Tests:** `tst_qml_smoke::settingsGeneralLoads` checks the line.

**Files:** `qml/GeneralSettings.qml`, `tests/tst_qml_smoke.cpp`
