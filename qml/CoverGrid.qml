import QtQuick
import QtQuick.Controls
import Tyli

// Cards in a grid that reflows with the width (at least 2 columns). The view pages its
// model as it scrolls (GroupModel.fetchMore). `title` / `subtitle` / `activated` are
// functions of a row: the screen decides what a group means.
GridView {
    id: grid

    property bool poster: false
    property string placeholder: "music"
    property var titleOf: (row) => row.values[0]
    property var subtitleOf: (row) => ""
    signal opened(var row)

    readonly property int minCard: 150
    readonly property int gap: 18
    readonly property int columns: Math.max(2, Math.floor((width + gap) / (minCard + gap)))
    readonly property real cardWidth: (width - (columns - 1) * gap) / columns

    clip: true
    boundsBehavior: Flickable.StopAtBounds
    cellWidth: cardWidth + gap
    cellHeight: (poster ? cardWidth * 1.5 : cardWidth) + 7 + 2 * Theme.fontSize + 18
    ScrollBar.vertical: TyScrollBar {}

    delegate: Item {
        id: cell
        required property var model
        required property int index
        width: grid.cellWidth
        height: grid.cellHeight
        CoverCard {
            width: grid.cardWidth
            poster: grid.poster
            placeholder: grid.placeholder
            source: cell.model.thumbnail
            online: cell.model.online
            title: grid.titleOf(cell.model)
            subtitle: grid.subtitleOf(cell.model)
            onActivated: grid.opened(cell.model)
        }
    }
}
