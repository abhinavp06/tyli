import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Every hoardor setting, in four sub-tabs.
Item {
    id: settingsView

    property alias currentIndex: subTabs.currentIndex

    ColumnLayout {
        anchors.fill: parent
        spacing: 0
        TabBar {
            id: subTabs
            objectName: "settingsTabs"
            Layout.fillWidth: true
            TabButton { text: "Folders"; width: implicitWidth + 24 }
            TabButton { text: "Categories"; width: implicitWidth + 24 }
            TabButton { text: "Scanning"; width: implicitWidth + 24 }
            TabButton { text: "Sync"; width: implicitWidth + 24 }
        }
        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: subTabs.currentIndex
            FolderSettings {}
            CategorySettings {}
            ScanningSettings {}
            SyncSettings {}
        }
    }
}
