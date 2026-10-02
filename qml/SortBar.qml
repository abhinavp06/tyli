import QtQuick
import Tyli

// The sort choices every grid shares (features/media_listing_ui.md §1). "added" is one
// button: the first click sorts newest first, the next flips to oldest first.
Row {
    id: bar

    // [{key, label}]; the "added" key toggles direction.
    property var options: [{ key: "added", label: "added" }, { key: "az", label: "a–z" }, { key: "year", label: "year" }]
    property string sortKey: "added"
    property bool addedDescending: true
    signal picked(string key, bool addedDescending)

    spacing: 16

    Text {
        anchors.verticalCenter: parent.verticalCenter
        text: "sort"
        color: Theme.textDim
        font.pixelSize: Theme.fontSizeSmall
    }
    Repeater {
        model: bar.options
        Text {
            required property var modelData
            readonly property bool on: bar.sortKey === modelData.key
            objectName: "sort_" + modelData.key
            anchors.verticalCenter: parent.verticalCenter
            text: modelData.key === "added" ? "added " + (bar.addedDescending ? "↓" : "↑") : modelData.label
            color: on ? Theme.accent : area.containsMouse ? Theme.text : Theme.textDim
            font.pixelSize: Theme.fontSizeSmall
            MouseArea {
                id: area
                anchors.fill: parent
                anchors.margins: -4
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    const flip = modelData.key === "added" && parent.on
                    bar.picked(modelData.key, flip ? !bar.addedDescending : (modelData.key === "added" ? true : bar.addedDescending))
                }
            }
        }
    }
}
