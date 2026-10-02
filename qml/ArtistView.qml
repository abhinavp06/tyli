import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Tyli
import "Format.js" as Format

// An artist: their albums (album artist = them), then the albums of others they appear on.
ColumnLayout {
    id: page
    objectName: "artistView"

    property var owner
    readonly property string name: owner.route.params.name || ""

    spacing: 0

    GroupModel {
        id: own
        engine: "audio"
        groupBy: ["albumArtist", "album"]
        filter: ({ category: page.owner.categoryId, albumArtist: page.name })
        order: "year"
    }
    GroupModel {
        id: appearances
        engine: "audio"
        groupBy: ["albumArtist", "album"]
        filter: ({ category: page.owner.categoryId, artist: page.name })
        order: "year"
    }

    BrowseHeader {
        Layout.fillWidth: true
        crumbs: page.owner.crumbs()
        label: "artist"
        title: page.name
        subtitle: Format.plural(own.total, "album")
        showSort: false
        onBack: page.owner.back()
        onCrumbClicked: (crumb) => page.owner.goToCrumb(crumb)
    }

    Flickable {
        Layout.fillWidth: true
        Layout.fillHeight: true
        clip: true
        contentHeight: content.implicitHeight + 24
        boundsBehavior: Flickable.StopAtBounds
        ScrollBar.vertical: TyScrollBar {}

        Column {
            id: content
            width: parent.width
            topPadding: 18
            spacing: 14

            component Albums: Flow {
                property alias model: repeater.model
                property bool skipOwn: false
                width: content.width
                spacing: 18
                Repeater {
                    id: repeater
                    CoverCard {
                        required property var model
                        // "Appears on" leaves out the artist's own albums.
                        visible: !parent.skipOwn || model.values[0].toLowerCase() !== page.name.toLowerCase()
                        width: visible ? 150 : 0
                        source: model.thumbnail
                        online: model.online
                        title: model.values[1]
                        subtitle: (model.yearMax > 0 ? model.yearMax + " · " : "") + (parent.skipOwn ? model.values[0] : Format.plural(model.count, "track"))
                        onActivated: page.owner.open("album", { albumArtist: model.values[0], album: model.values[1], coverEntry: model.coverEntry, crumb: model.values[1] })
                    }
                }
            }

            Text { text: "albums"; color: Theme.textDim; font.pixelSize: Theme.fontSizeSmall; visible: own.count > 0 }
            Albums { model: own }
            Text {
                text: "appears on"
                color: Theme.textDim
                font.pixelSize: Theme.fontSizeSmall
                visible: appearances.total > own.total
                topPadding: 10
            }
            Albums { model: appearances; skipOwn: true }
        }
    }
}
