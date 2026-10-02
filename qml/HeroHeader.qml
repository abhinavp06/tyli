import QtQuick
import QtQuick.Layouts
import Tyli

// The top of an album, movie, or show page: the art, a small label, the title, and the
// facts (put in `facts`). Actions sit at the top right (`actions`), as everywhere in TYLI.
Item {
    id: hero

    property alias source: art.source
    property alias placeholder: art.placeholder
    property bool online: true
    property bool poster: false
    property string label
    property string title
    property int artWidth: poster ? 160 : 220
    default property alias facts: factColumn.data
    property alias actions: actionRow.data

    implicitHeight: Math.max(art.height, factColumn.implicitHeight)

    CoverImage {
        id: art
        width: hero.artWidth
        height: hero.poster ? Math.round(hero.artWidth * 1.5) : hero.artWidth
        online: hero.online
    }
    ColumnLayout {
        id: factColumn
        anchors.left: art.right
        anchors.leftMargin: 28
        anchors.right: actionRow.left
        anchors.rightMargin: 16
        anchors.bottom: art.bottom
        spacing: 8
        Text { text: hero.label; color: Theme.textDim; font.pixelSize: Theme.fontSizeSmall; font.letterSpacing: 1 }
        Text {
            Layout.fillWidth: true
            text: hero.title
            color: Theme.text
            font.pixelSize: 26
            font.weight: Theme.weightBold
            wrapMode: Text.Wrap
            maximumLineCount: 2
            elide: Text.ElideRight
        }
    }
    Row {
        id: actionRow
        anchors.top: parent.top
        anchors.right: parent.right
        spacing: 10
    }
}
