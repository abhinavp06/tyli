import QtQuick
import QtQuick.Controls
import Tyli
import "Format.js" as Format

// Albums of a category (plus any extra filter: a genre, a year) in the chosen sort:
// "added" (newest or oldest first), "az" (by album name), "year", or "artist" (sections
// under each album artist). An album is a group of tracks by (album artist, album).
Item {
    id: browser

    property var owner
    property var filter: ({})         // the category, plus a genre or a year
    property string sortKey: "added"
    property bool addedDescending: true
    readonly property int total: sortKey === "artist" ? artistAlbums.total : albums.total

    function openAlbum(artist, album, coverEntry) {
        owner.open("album", { albumArtist: artist, album: album, coverEntry: coverEntry, crumb: album })
    }

    GroupModel {
        id: albums
        engine: "audio"
        groupBy: browser.sortKey === "az" ? ["album", "albumArtist"] : ["albumArtist", "album"]
        filter: browser.filter
        order: browser.sortKey === "added" ? "added" : browser.sortKey === "year" ? "year" : "values"
        descending: browser.sortKey === "added" ? browser.addedDescending : browser.sortKey === "year"
    }
    GroupModel {
        id: artistAlbums  // only for the total in "artist" mode
        engine: "audio"
        groupBy: browser.sortKey === "artist" ? ["albumArtist", "album"] : []
        filter: browser.filter
    }

    CoverGrid {
        objectName: "albumGrid"
        anchors.fill: parent
        anchors.topMargin: Theme.px(18)
        visible: browser.sortKey !== "artist"
        model: browser.sortKey !== "artist" ? albums : null
        placeholder: "music"
        readonly property bool albumFirst: browser.sortKey === "az"
        titleOf: (row) => albumFirst ? row.values[0] : row.values[1]
        subtitleOf: (row) => (albumFirst ? row.values[1] : row.values[0]) + (row.yearMax > 0 ? " · " + row.yearMax : "")
        onOpened: (row) => browser.openAlbum(albumFirst ? row.values[1] : row.values[0], albumFirst ? row.values[0] : row.values[1], row.coverEntry)
    }

    // "artist": one section per album artist, their albums by year.
    ListView {
        objectName: "artistSections"
        anchors.fill: parent
        anchors.topMargin: Theme.px(12)
        visible: browser.sortKey === "artist"
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        spacing: Theme.px(22)
        ScrollBar.vertical: TyScrollBar {}
        model: GroupModel {
            engine: "audio"
            groupBy: browser.sortKey === "artist" ? ["albumArtist"] : []
            filter: browser.filter
        }
        delegate: Column {
            id: section
            required property var model
            width: ListView.view.width
            spacing: Theme.px(12)
            Row {
                spacing: Theme.px(12)
                LinkText {
                    id: artistName
                    text: section.model.values[0]
                    font.weight: Theme.weightBold
                    onActivated: browser.owner.open("artist", { name: section.model.values[0], crumb: section.model.values[0] })
                }
                Text { anchors.baseline: artistName.baseline; text: Format.plural(artistModel.total, "album"); color: Theme.textDim; font.pixelSize: Theme.fontSizeSmall }
            }
            Rectangle { width: parent.width; height: 1; color: Theme.hairline }
            CardFlow {
                width: parent.width
                Repeater {
                    model: GroupModel {
                        id: artistModel
                        engine: "audio"
                        groupBy: ["albumArtist", "album"]
                        filter: Object.assign({ albumArtist: section.model.values[0] }, browser.filter)
                        order: "year"
                    }
                    CoverCard {
                        required property var model
                        width: parent.cardWidth
                        source: model.thumbnail
                        online: model.online
                        title: model.values[1]
                        subtitle: (model.yearMax > 0 ? model.yearMax + " · " : "") + Format.plural(model.count, "track")
                        onActivated: browser.openAlbum(model.values[0], model.values[1], model.coverEntry)
                    }
                }
            }
        }
    }
}
