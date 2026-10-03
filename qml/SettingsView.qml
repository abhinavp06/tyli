import QtQuick
import QtQuick.Layouts
import Tyli

// Settings: `library` (categories, folders, sync) and `general` (scanning, sync behavior, about).
Item {
    id: view
    objectName: "settingsView"

    property string currentPage: "library"
    property int focusedCategoryId: -1  // the category open in the main view; Library starts on it

    onVisibleChanged: if (visible) library.selectedCategoryId = focusedCategoryId
    function showCategory(categoryId) { library.selectedCategoryId = categoryId }

    ColumnLayout {
        anchors.fill: parent
        anchors.leftMargin: Theme.pageMargin
        anchors.rightMargin: Theme.pageMargin
        anchors.topMargin: Theme.px(20)
        anchors.bottomMargin: Theme.px(12)
        spacing: Theme.px(14)

        Text {
            text: "settings"
            color: Theme.text
            font.pixelSize: Theme.fontSizeTitle
            font.weight: Theme.weightBold
        }

        Item {
            Layout.fillWidth: true
            implicitHeight: tabs.implicitHeight
            Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hairline }
            Row {
                id: tabs
                objectName: "settingsTabs"
                TyTab { text: "library"; checked: view.currentPage === "library"; onClicked: view.currentPage = "library" }
                TyTab { text: "general"; checked: view.currentPage === "general"; onClicked: view.currentPage = "general" }
            }
        }

        LibrarySettings {
            id: library
            objectName: "librarySettings"
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: view.currentPage === "library"
        }
        GeneralSettings {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: view.currentPage === "general"
        }
    }
}
