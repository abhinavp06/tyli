import QtQuick
import QtQuick.Controls

ApplicationWindow {
    width: 960
    height: 600
    visible: true
    title: "TYLI"

    Label {
        anchors.centerIn: parent
        text: "TYLI skeleton · hoardor categories: " + hoardorCategories.join(", ")
    }
}
