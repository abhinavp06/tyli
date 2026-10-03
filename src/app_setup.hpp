#pragma once

namespace tyli {

// The app-wide look, before any QML loads: the Basic Qt Quick Controls style (which
// TYLI's own controls restyle completely) and the bundled IBM Plex Mono as the default
// font, and mouse-wheel scrolling wheelSpeed() times as far as Qt's default. Used by main()
// and by the QML tests, so both look and scroll the same.
void setupApplication();

// How far one wheel notch scrolls, as a multiple of Qt's default (the system's lines per
// notch × 24 px). The "scroll speed" preference sets it (LibraryBridge::setScrollSpeed);
// UI thread only. The user found 1.2× still slow (2026-10-03): 2× by default.
inline constexpr int DefaultScrollSpeedPercent = 200;
void setWheelSpeed(double factor);
double wheelSpeed();

}
