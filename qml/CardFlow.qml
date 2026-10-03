import QtQuick
import Tyli

// A row of cards that wraps and fills its width edge to edge, sized like CoverGrid's: as many
// columns of at least `minCard` as fit, then stretched, so the last card ends at the right
// edge (aligned with the header's sort and actions). Cards take `width: parent.cardWidth`.
Flow {
    readonly property int minCard: Theme.px(150)
    readonly property int gap: Theme.px(18)
    readonly property int columns: Math.max(2, Math.floor((width + gap) / (minCard + gap)))
    // Rounded down a hair, so float sums never push the last card onto the next line.
    readonly property real cardWidth: Math.floor((width - (columns - 1) * gap) / columns * 100) / 100

    spacing: gap
}
