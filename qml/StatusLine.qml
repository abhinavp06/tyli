import QtQuick
import QtQuick.Layouts
import Tyli

// One line at the bottom of the window for results and errors (no popups). Results
// clear themselves after a few seconds; errors stay until dismissed.
Rectangle {
    id: line

    property string message
    property bool isError: false

    function show(text, error) {
        message = text
        isError = error
        if (error)
            clearTimer.stop()
        else
            clearTimer.restart()
    }

    implicitHeight: message !== "" ? 30 : 0
    visible: message !== ""
    color: Theme.background

    Rectangle { width: parent.width; height: 1; color: Theme.hairline }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 16
        anchors.rightMargin: 8
        spacing: 12
        Text {
            objectName: "statusText"
            Layout.fillWidth: true
            text: line.message
            color: line.isError ? Theme.error : Theme.textDim
            elide: Text.ElideRight
        }
        TyIconButton {
            implicitHeight: 26
            implicitWidth: 26
            iconName: "close"
            tip: "dismiss"
            onClicked: line.message = ""
        }
    }

    Timer {
        id: clearTimer
        interval: 8000
        onTriggered: line.message = ""
    }

    Connections {
        target: Library
        function onErrorOccurred(message) { line.show(message, true) }
        function onFolderOperationFinished(ok, message) { line.show(message, !ok) }
    }
}
