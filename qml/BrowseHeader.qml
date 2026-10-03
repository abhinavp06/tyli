import QtQuick
import QtQuick.Layouts
import Tyli

// The top of every grid page: an optional back arrow and breadcrumb, a small label
// ("artist", "genre"), the title, its counts, and the shared sort bar on the right.
ColumnLayout {
    id: header

    property var crumbs: []            // [{text, page, params}]; the last one is the current page (not a link)
    property string label
    property string title
    property string subtitle
    property alias sortOptions: sort.options
    property alias sortKey: sort.sortKey
    property alias addedDescending: sort.addedDescending
    property bool showSort: true
    signal back()
    signal crumbClicked(var crumb)
    signal sortPicked(string key, bool addedDescending)

    spacing: Theme.px(10)

    RowLayout {
        visible: header.crumbs.length > 1
        spacing: Theme.px(8)
        TyIconButton { objectName: "backButton"; iconName: "back"; tip: "back"; onClicked: header.back() }
        Repeater {
            model: header.crumbs
            Row {
                required property var modelData
                required property int index
                spacing: Theme.px(8)
                LinkText {
                    visible: index < header.crumbs.length - 1
                    text: modelData.text
                    color: Theme.textDim
                    onActivated: header.crumbClicked(modelData)
                }
                Text { visible: index === header.crumbs.length - 1; text: modelData.text; color: Theme.text }
                Text { visible: index < header.crumbs.length - 1; text: "/"; color: Theme.textFaint }
            }
        }
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: Theme.px(16)
        ColumnLayout {
            spacing: 2
            Text { visible: header.label !== ""; text: header.label; color: Theme.textDim; font.pixelSize: Theme.fontSizeSmall; font.letterSpacing: 1 }
            Text {
                text: header.title
                color: Theme.text
                font.pixelSize: header.label !== "" ? Theme.px(24) : Theme.fontSizeTitle
                font.weight: Theme.weightBold
            }
        }
        Text { Layout.alignment: Qt.AlignBottom; Layout.bottomMargin: Theme.px(3); text: header.subtitle; color: Theme.textDim }
        Item { Layout.fillWidth: true }
        SortBar {
            id: sort
            Layout.alignment: Qt.AlignBottom
            Layout.bottomMargin: Theme.px(4)
            visible: header.showSort
            onPicked: (key, desc) => header.sortPicked(key, desc)
        }
    }
    Rectangle { Layout.fillWidth: true; height: 1; color: Theme.hairline }
}
