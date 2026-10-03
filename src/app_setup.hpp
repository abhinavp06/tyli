#pragma once

namespace tyli {

// The app-wide look, before any QML loads: the Basic Qt Quick Controls style (which
// TYLI's own controls restyle completely) and the bundled IBM Plex Mono as the default
// font, and mouse-wheel scrolling WheelSpeed times as fast as Qt's default. Used by main()
// and by the QML tests, so both look and scroll the same.
void setupApplication();

// The user found scrolling a little slow (2026-10-03): 20% further per wheel notch.
inline constexpr double WheelSpeed = 1.2;

}
