#pragma once

namespace tyli {

// The app-wide look, before any QML loads: the Basic Qt Quick Controls style (which
// TYLI's own controls restyle completely) and the bundled IBM Plex Mono as the default
// font. Used by main() and by the QML tests, so both look the same.
void setupApplication();

}
