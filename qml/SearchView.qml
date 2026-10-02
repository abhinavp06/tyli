import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Tyli
import "Format.js" as Format

// Results for the top bar's search, across every category: albums, artists, tracks,
// movies, shows. Each opens its page in its category (`opened(categoryId, page, params)`).
// hoardor matches every word as a prefix, ignoring case and accents.
Item {
    id: view
    objectName: "searchView"

    property string text
    signal opened(int categoryId, string page, var params)

    readonly property var audioFilter: ({ search: view.text })
    readonly property int shown: 12  // per section

    GroupModel { id: albums; engine: "audio"; groupBy: ["category", "albumArtist", "album"]; filter: view.audioFilter }
    GroupModel { id: artists; engine: "audio"; groupBy: ["category", "albumArtist"]; filter: view.audioFilter }
    ItemModel { id: tracks; engine: "audio"; filter: view.audioFilter; order: ["title"] }
    GroupModel { id: movies; engine: "video"; groupBy: ["category", "title", "year"]; filter: ({ search: view.text, type: 1 }) }
    GroupModel { id: shows; engine: "video"; groupBy: ["category", "show"]; filter: ({ search: view.text, type: 2 }) }

    readonly property int found: albums.total + artists.total + tracks.count + movies.total + shows.total

    component Section: Column {
        property string title
        property int total
        width: content.width
        spacing: 12
        visible: total > 0
        Row {
            spacing: 12
            Text { text: parent.parent.title; color: Theme.text; font.weight: Theme.weightBold }
            Text { text: Format.count(parent.parent.total); color: Theme.textDim; font.pixelSize: Theme.fontSizeSmall; anchors.baseline: parent.children[0].baseline }
        }
        Rectangle { width: parent.width; height: 1; color: Theme.hairline }
    }

    Flickable {
        anchors.fill: parent
        anchors.leftMargin: Theme.pageMargin
        anchors.rightMargin: Theme.pageMargin
        anchors.topMargin: 20
        clip: true
        contentHeight: content.implicitHeight + 24
        boundsBehavior: Flickable.StopAtBounds
        ScrollBar.vertical: TyScrollBar {}

        Column {
            id: content
            width: parent.width
            spacing: 22

            Row {
                spacing: 16
                Text { text: "search"; color: Theme.text; font.pixelSize: Theme.fontSizeTitle; font.weight: Theme.weightBold }
                Text {
                    anchors.baseline: parent.children[0].baseline
                    text: view.found > 0 ? "“" + view.text + "”" : "nothing matches “" + view.text + "”"
                    color: Theme.textDim
                }
            }

            Section { title: "albums"; total: albums.total }
            Flow {
                width: content.width
                spacing: 18
                visible: albums.total > 0
                Repeater {
                    model: albums
                    CoverCard {
                        required property var model
                        required property int index
                        visible: index < view.shown
                        width: visible ? 150 : 0
                        source: model.thumbnail
                        online: model.online
                        title: model.values[2]
                        subtitle: model.values[1] + (model.yearMax > 0 ? " · " + model.yearMax : "")
                        onActivated: view.opened(Number(model.values[0]), "album",
                                                 { albumArtist: model.values[1], album: model.values[2], coverEntry: model.coverEntry, crumb: model.values[2] })
                    }
                }
            }

            Section { title: "artists"; total: artists.total }
            Flow {
                width: content.width
                spacing: 18
                visible: artists.total > 0
                Repeater {
                    model: artists
                    LinkText {
                        required property var model
                        required property int index
                        visible: index < view.shown * 2
                        text: model.values[1]
                        onActivated: view.opened(Number(model.values[0]), "artist", { name: model.values[1], crumb: model.values[1] })
                    }
                }
            }

            Section { title: "tracks"; total: tracks.count }
            Column {
                width: content.width
                visible: tracks.count > 0
                Repeater {
                    model: tracks
                    Rectangle {
                        required property var model
                        required property int index
                        visible: index < view.shown
                        width: content.width
                        height: visible ? 32 : 0
                        color: trackArea.containsMouse ? Theme.hover : "transparent"
                        Row {
                            anchors.verticalCenter: parent.verticalCenter
                            leftPadding: 12
                            spacing: 16
                            Text { width: 360; text: model.title; elide: Text.ElideRight; color: model.online ? Theme.text : Theme.textFaint }
                            Text { width: 260; text: model.albumArtist; elide: Text.ElideRight; color: Theme.textDim }
                            Text { width: 260; text: model.album; elide: Text.ElideRight; color: Theme.textDim }
                        }
                        MouseArea {
                            id: trackArea
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            // A track opens its album (playing comes with the player).
                            onClicked: view.opened(Library.categoryOfFolder(model.rootId), "album",
                                                   { albumArtist: model.albumArtist, album: model.album, crumb: model.album })
                        }
                    }
                }
            }

            Section { title: "movies"; total: movies.total }
            Flow {
                width: content.width
                spacing: 18
                visible: movies.total > 0
                Repeater {
                    model: movies
                    CoverCard {
                        required property var model
                        required property int index
                        visible: index < view.shown
                        width: visible ? 130 : 0
                        poster: true
                        placeholder: "movies"
                        source: model.thumbnail
                        online: model.online
                        title: model.values[1]
                        subtitle: model.values[2] > 0 ? model.values[2] : ""
                        onActivated: view.opened(Number(model.values[0]), "movie",
                                                 { title: model.values[1], year: Number(model.values[2]), coverEntry: model.coverEntry, crumb: model.values[1] })
                    }
                }
            }

            Section { title: "shows"; total: shows.total }
            Flow {
                width: content.width
                spacing: 18
                visible: shows.total > 0
                Repeater {
                    model: shows
                    CoverCard {
                        required property var model
                        required property int index
                        visible: index < view.shown
                        width: visible ? 130 : 0
                        poster: true
                        placeholder: "shows"
                        source: model.thumbnail
                        online: model.online
                        title: model.values[1]
                        subtitle: Format.plural(model.count, "episode")
                        onActivated: view.opened(Number(model.values[0]), "show", { show: model.values[1], coverEntry: model.coverEntry, crumb: model.values[1] })
                    }
                }
            }
        }
    }
}
