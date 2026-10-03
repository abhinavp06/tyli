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

    implicitHeight: message !== "" ? Theme.px(30) : 0
    visible: message !== ""
    color: Theme.background

    Rectangle { width: parent.width; height: 1; color: Theme.hairline }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: Theme.px(16)
        anchors.rightMargin: Theme.px(8)
        spacing: Theme.px(12)
        Text {
            objectName: "statusText"
            Layout.fillWidth: true
            text: line.message
            color: line.isError ? Theme.error : Theme.textDim
            elide: Text.ElideRight
        }
        TyIconButton {
            implicitHeight: Theme.px(26)
            implicitWidth: Theme.px(26)
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
        function onNotice(message) { line.show(message, false) }
        function onMetadataFinished(report) {
            if (report.read + report.failed === 0)
                return
            line.show("read " + report.read + (report.read === 1 ? " file" : " files")
                      + (report.failed > 0 ? " · " + report.failed + " unreadable" : "")
                      + (report.cancelled ? " · stopped, the rest waits for the next sync" : ""), false)
        }
    }
}
