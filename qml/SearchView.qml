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

    readonly property int shown: 12  // per section (artists: twice as many)
    readonly property int minLength: 2   // one letter matches most of a library: too slow, never useful

    // The latest results from Library.search (off the UI thread). They stay while the page is
    // hidden, so going back from a result shows them again as they were.
    property string answered: ""    // the text `results` answer
    property var results: ({})
    readonly property var albums: results.albums || { total: 0, items: [] }
    readonly property var artists: results.artists || { total: 0, items: [] }
    readonly property var tracks: results.tracks || { total: 0, items: [] }
    readonly property var movies: results.movies || { total: 0, items: [] }
    readonly property var shows: results.shows || { total: 0, items: [] }
    readonly property int found: albums.total + artists.total + tracks.total + movies.total + shows.total
    readonly property bool tooShort: text.length > 0 && text.length < minLength

    onTextChanged: if (text.length >= minLength && text !== answered) Library.search(text)
    Connections {
        target: Library
        function onSearchFinished(text, results) {
            if (text !== view.text) return
            view.answered = text
            view.results = results
        }
    }

    component Section: Column {
        property string title
        property int total
        width: content.width
        spacing: Theme.px(12)
        visible: total > 0
        Row {
            spacing: Theme.px(12)
            Text { text: parent.parent.title; color: Theme.text; font.weight: Theme.weightBold }
            Text { text: Format.count(parent.parent.total); color: Theme.textDim; font.pixelSize: Theme.fontSizeSmall; anchors.baseline: parent.children[0].baseline }
        }
        Rectangle { width: parent.width; height: 1; color: Theme.hairline }
    }

    Flickable {
        anchors.fill: parent
        anchors.leftMargin: Theme.pageMargin
        anchors.rightMargin: Theme.pageMargin
        anchors.topMargin: Theme.px(20)
        clip: true
        contentHeight: content.implicitHeight + Theme.px(24)
        boundsBehavior: Flickable.StopAtBounds
        ScrollBar.vertical: TyScrollBar {}

        Column {
            id: content
            width: parent.width
            spacing: Theme.px(22)

            Row {
                spacing: Theme.px(16)
                Text { text: "search"; color: Theme.text; font.pixelSize: Theme.fontSizeTitle; font.weight: Theme.weightBold }
                Text {
                    anchors.baseline: parent.children[0].baseline
                    text: view.tooShort ? "type at least " + view.minLength + " letters"
                        : view.answered !== view.text ? "searching “" + view.text + "”…"
                        : view.found > 0 ? "“" + view.text + "”" : "nothing matches “" + view.text + "”"
                    color: Theme.textDim
                }
            }

            Section { title: "albums"; total: view.albums.total }
            Flow {
                width: content.width
                spacing: Theme.px(18)
                visible: view.albums.total > 0
                Repeater {
                    model: view.albums.items
                    CoverCard {
                        required property var modelData
                        readonly property var model: modelData
                        width: Theme.px(150)
                        source: Library.thumbnail(model.coverEntry)
                        online: model.online
                        title: model.values[2]
                        subtitle: model.values[1] + (model.yearMax > 0 ? " · " + model.yearMax : "")
                        onActivated: view.opened(Number(model.values[0]), "album",
                                                 { albumArtist: model.values[1], album: model.values[2], coverEntry: model.coverEntry, crumb: model.values[2] })
                    }
                }
            }

            Section { title: "artists"; total: view.artists.total }
            Flow {
                width: content.width
                spacing: Theme.px(18)
                visible: view.artists.total > 0
                Repeater {
                    model: view.artists.items
                    LinkText {
                        required property var modelData
                        readonly property var model: modelData
                        text: model.values[1]
                        onActivated: view.opened(Number(model.values[0]), "artist", { name: model.values[1], crumb: model.values[1] })
                    }
                }
            }

            Section { title: "tracks"; total: view.tracks.total }
            Column {
                width: content.width
                visible: view.tracks.total > 0
                Repeater {
                    model: view.tracks.items
                    Rectangle {
                        required property var modelData
                        readonly property var model: modelData
                        width: content.width
                        height: Theme.px(32)
                        color: trackArea.containsMouse ? Theme.hover : "transparent"
                        Row {
                            anchors.verticalCenter: parent.verticalCenter
                            leftPadding: Theme.px(12)
                            spacing: Theme.px(16)
                            Text { width: Theme.px(360); text: model.title; elide: Text.ElideRight; color: model.online ? Theme.text : Theme.textFaint }
                            Text { width: Theme.px(260); text: model.albumArtist; elide: Text.ElideRight; color: Theme.textDim }
                            Text { width: Theme.px(260); text: model.album; elide: Text.ElideRight; color: Theme.textDim }
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

            Section { title: "movies"; total: view.movies.total }
            Flow {
                width: content.width
                spacing: Theme.px(18)
                visible: view.movies.total > 0
                Repeater {
                    model: view.movies.items
                    CoverCard {
                        required property var modelData
                        readonly property var model: modelData
                        width: Theme.px(130)
                        poster: true
                        placeholder: "movies"
                        source: Library.thumbnail(model.coverEntry)
                        online: model.online
                        title: model.values[1]
                        subtitle: model.values[2] > 0 ? model.values[2] : ""
                        onActivated: view.opened(Number(model.values[0]), "movie",
                                                 { title: model.values[1], year: Number(model.values[2]), coverEntry: model.coverEntry, crumb: model.values[1] })
                    }
                }
            }

            Section { title: "shows"; total: view.shows.total }
            Flow {
                width: content.width
                spacing: Theme.px(18)
                visible: view.shows.total > 0
                Repeater {
                    model: view.shows.items
                    CoverCard {
                        required property var modelData
                        readonly property var model: modelData
                        width: Theme.px(130)
                        poster: true
                        placeholder: "shows"
                        source: Library.thumbnail(model.coverEntry)
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
