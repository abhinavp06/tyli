import QtQuick
import Tyli

// A cached thumbnail (never read from a media drive), or a placeholder icon. Dimmed when
// its drive is offline.
Rectangle {
    id: cover

    property string source         // a file: URL from the thumbnail cache, or ""
    property string placeholder: "music"
    property bool online: true

    color: "transparent"
    border.width: image.status === Image.Ready ? 0 : 1
    border.color: Theme.hairline
    opacity: online ? 1 : 0.35

    Image {
        id: image
        anchors.fill: parent
        source: cover.source
        fillMode: Image.PreserveAspectCrop
        asynchronous: true
        cache: true
        sourceSize.width: 320
    }
    Icon {
        anchors.centerIn: parent
        visible: image.status !== Image.Ready
        name: cover.placeholder
        size: 28
        color: Theme.textFaint
    }
}
