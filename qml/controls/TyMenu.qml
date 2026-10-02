import QtQuick
import QtQuick.Templates as T
import Tyli

// A popup menu: black with a hairline border; items are TyMenuItem.
T.Menu {
    id: control

    implicitWidth: Math.max(160, contentWidth + leftPadding + rightPadding)
    implicitHeight: contentHeight + topPadding + bottomPadding
    padding: 1
    margins: 0

    delegate: TyMenuItem {}

    contentItem: ListView {
        implicitHeight: contentHeight
        implicitWidth: contentItem.childrenRect.width
        model: control.contentModel
        interactive: Window.window ? contentHeight > Window.window.height : false
        clip: true
        currentIndex: control.currentIndex
    }

    background: Rectangle {
        color: Theme.background
        border.width: 1
        border.color: Theme.hairline
    }
}
