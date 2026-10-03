import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Tyli
import "Format.js" as Format

// One movie, laid out like an album page (mockup movie-album.html): the poster and facts (year,
// length, genres, directors, copies, drive; links), quality (best copy by default) and play at
// the top right, the plot, then its copies as rows (like an album's tracks), and rows of more
// by its directors and more in its genres.
ColumnLayout {
    id: page
    objectName: "movieView"

    property var owner
    property var route: ({ page: "home", params: {} })  // this page's own: set by CategoryView
    property int depth: 0
    readonly property var params: route.params.title !== undefined ? route.params : ({ title: "", year: 0 })
    property var copies: Library.movieCopies(params.title, params.year, owner.categoryId)
    readonly property string qualityKey: "quality/movie/" + encodeURIComponent(params.title + "\u001f" + params.year)
    property string chosen: Library.preference(qualityKey, "")
    readonly property int copyIndex: Math.max(0, copies.findIndex(c => c.label === chosen))
    readonly property var item: copies[copyIndex] ? copies[copyIndex].item : null
    // As many cards a "more" row as fit (CardFlow's columns, at this page's width).
    readonly property int perRow: Math.max(2, Math.floor((width + Theme.px(18)) / (Theme.px(150) + Theme.px(18))))
    property var more: Library.moreMovies(params.title, params.year, owner.categoryId, perRow)

    function choose(index) {
        page.chosen = page.copies[index].label
        Library.setPreference(page.qualityKey, page.chosen)
    }
    // "TrueHD 7.1 eng · AC3 5.1 deu · subs eng, deu"
    function streamsText(item) {
        const channels = (n) => n === 1 ? "1.0" : n === 2 ? "2.0" : n > 2 ? (n - 1) + ".1" : ""
        const audio = (item.audioStreams || []).map(a => [a.codec.toUpperCase(), channels(a.channels), a.language].filter(x => x).join(" "))
        const subs = Format.unique((item.subtitleStreams || []).map(s => s.language || s.codec))
        return audio.concat(subs.length > 0 ? ["subs " + subs.join(", ")] : []).join(" · ")
    }

    Connections {
        target: Library
        function onContentChanged() {
            page.copies = Library.movieCopies(page.params.title, page.params.year, page.owner.categoryId)
            page.more = Library.moreMovies(page.params.title, page.params.year, page.owner.categoryId, page.perRow)
        }
    }

    spacing: Theme.px(18)

    BrowseHeader {
        Layout.fillWidth: true
        crumbs: page.owner.crumbs(page.depth)
        showSort: false
        onBack: page.owner.back()
        onCrumbClicked: (crumb) => page.owner.goToCrumb(crumb)
    }

    HeroHeader {
        Layout.fillWidth: true
        poster: true
        artWidth: Theme.px(147)  // as tall as an album cover (220)
        placeholder: "movies"
        source: Library.thumbnail(page.params.coverEntry || (page.item ? page.item.posterEntry : 0))
        online: page.item ? page.item.online : true
        label: "movie"
        title: page.params.title

        Flow {
            Layout.fillWidth: true
            spacing: Theme.px(14)
            LinkText {
                visible: page.params.year > 0
                text: page.params.year
                onActivated: page.owner.open("year", { value: page.params.year, crumb: String(page.params.year) })
            }
            Text { visible: page.item !== null && page.item.duration > 0; text: page.item ? Format.runtime(page.item.duration) : ""; color: Theme.textDim }
            Repeater {
                model: page.item ? page.item.genres : []
                LinkText {
                    required property string modelData
                    text: modelData
                    onActivated: page.owner.open("genre", { value: modelData, crumb: modelData })
                }
            }
        }
        Flow {
            Layout.fillWidth: true
            spacing: Theme.px(14)
            visible: page.item !== null && page.item.directors.length > 0
            Text { text: "directed by"; color: Theme.textDim; font.pixelSize: Theme.fontSizeSmall }
            Repeater {
                model: page.item ? page.item.directors : []
                LinkText {
                    required property string modelData
                    text: modelData
                    onActivated: page.owner.open("director", { name: modelData, crumb: modelData })
                }
            }
        }
        Row {
            spacing: Theme.px(14)
            visible: page.item !== null
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: (page.copies.length === 1 ? "1 copy" : page.copies.length + " copies") + " · " + Format.bytes(page.copies.reduce((t, c) => t + c.item.size, 0))
                color: Theme.textDim
            }
            Row {
                anchors.verticalCenter: parent.verticalCenter
                spacing: Theme.px(6)
                Icon { name: "drive"; size: Theme.px(14); anchors.verticalCenter: parent.verticalCenter }
                LinkText {
                    text: page.item ? Library.folderLabel(page.item.rootId) : ""
                    font.pixelSize: Theme.fontSizeSmall
                    onActivated: page.owner.folderSettingsRequested(page.item.rootId)
                }
            }
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: !page.item ? "" : page.item.source === "nfo" ? "info from its .nfo" : page.item.source === "tags" ? "info from its tags" : "info from its file name"
                color: Theme.textFaint
                font.pixelSize: Theme.fontSizeSmall
            }
        }

        actions: [
            QualityPicker {
                objectName: "qualityPicker"
                options: page.copies.map(c => ({ label: c.label, detail: Format.bytes(c.item.size) + (c.item.online ? "" : " · offline") }))
                currentIndex: page.copyIndex
                onPicked: (index) => page.choose(index)
            },
            PlayButton {
                objectName: "playMovie"
                // Re-read when playback saved a position (Player.revision).
                readonly property var saved: Player.revision >= 0 && page.item ? Player.itemStates([page.item.entryId])[0] : null
                readonly property bool resumes: saved && !saved.viewed && saved.position >= 60000
                tip: resumes ? "resume at " + Format.duration(saved.position) : saved && saved.viewed ? "play again (watched)" : "play"
                onClicked: if (page.item) page.owner.play([page.item.entryId], 0, [], "", true)
            }
        ]
    }

    Text {
        Layout.fillWidth: true
        Layout.maximumWidth: Theme.px(820)
        visible: text !== ""
        text: page.item ? page.item.plot : ""
        color: Theme.textDim
        wrapMode: Text.Wrap
        maximumLineCount: 5
        elide: Text.ElideRight
    }

    Rectangle { Layout.fillWidth: true; height: 1; color: Theme.hairline }

    Flickable {
        Layout.fillWidth: true
        Layout.fillHeight: true
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        contentHeight: below.implicitHeight
        ScrollBar.vertical: TyScrollBar {}

        Column {
            id: below
            width: parent.width

            // The copies, like an album's tracks: #, what it is (and its audio and subtitles),
            // length, size. The one that plays is green; a row plays that copy.
            readonly property real copyWidth: width - Theme.px(12 + 28 + 16 * 3 + 80 + 90 + 12)
            Row {
                height: Theme.px(30)
                spacing: Theme.px(16)
                leftPadding: Theme.px(12)
                Text { anchors.verticalCenter: parent.verticalCenter; width: Theme.px(28); horizontalAlignment: Text.AlignRight; text: "#"; color: Theme.textDim; font.pixelSize: Theme.fontSizeSmall }
                Text { anchors.verticalCenter: parent.verticalCenter; width: below.copyWidth; text: "copy"; color: Theme.textDim; font.pixelSize: Theme.fontSizeSmall }
                Text { anchors.verticalCenter: parent.verticalCenter; width: Theme.px(80); horizontalAlignment: Text.AlignRight; text: "length"; color: Theme.textDim; font.pixelSize: Theme.fontSizeSmall }
                Text { anchors.verticalCenter: parent.verticalCenter; width: Theme.px(90); horizontalAlignment: Text.AlignRight; text: "size"; color: Theme.textDim; font.pixelSize: Theme.fontSizeSmall }
            }
            Repeater {
                model: page.copies
                Rectangle {
                    id: copyRow
                    required property var modelData
                    required property int index
                    readonly property bool current: index === page.copyIndex
                    objectName: "copy-" + index
                    width: below.width
                    height: Theme.px(48)
                    color: copyArea.containsMouse ? Theme.hover : "transparent"
                    Rectangle { visible: copyRow.current; width: Theme.lineWidth; height: parent.height; color: Theme.accent }
                    Row {
                        height: parent.height
                        spacing: Theme.px(16)
                        leftPadding: Theme.px(12)
                        Text { anchors.verticalCenter: parent.verticalCenter; width: Theme.px(28); horizontalAlignment: Text.AlignRight; text: copyRow.index + 1; color: Theme.textDim }
                        Column {
                            anchors.verticalCenter: parent.verticalCenter
                            width: below.copyWidth
                            spacing: Theme.px(2)
                            Text {
                                width: parent.width
                                text: copyRow.modelData.label
                                elide: Text.ElideRight
                                color: copyRow.current ? Theme.accent : copyRow.modelData.item.online ? Theme.text : Theme.textFaint
                            }
                            Text {
                                width: parent.width
                                visible: text !== ""
                                text: page.streamsText(copyRow.modelData.item)
                                elide: Text.ElideRight
                                color: Theme.textDim
                                font.pixelSize: Theme.fontSizeSmall
                            }
                        }
                        Text { anchors.verticalCenter: parent.verticalCenter; width: Theme.px(80); horizontalAlignment: Text.AlignRight; text: copyRow.modelData.item.duration > 0 ? Format.duration(copyRow.modelData.item.duration) : ""; color: Theme.textDim }
                        Text { anchors.verticalCenter: parent.verticalCenter; width: Theme.px(90); horizontalAlignment: Text.AlignRight; text: Format.bytes(copyRow.modelData.item.size); color: Theme.textDim }
                    }
                    MouseArea {
                        id: copyArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            page.choose(copyRow.index)
                            page.owner.play([copyRow.modelData.item.entryId], 0, [], "", true)
                        }
                    }
                }
            }

            // More by each director, then more in each genre: one row of cards each.
            Repeater {
                model: page.more
                Column {
                    id: moreRow
                    required property var modelData
                    objectName: "more-" + modelData.kind + "-" + modelData.name
                    width: below.width
                    topPadding: Theme.px(26)
                    spacing: Theme.px(12)
                    function openAll() {
                        if (modelData.kind === "director")
                            page.owner.open("director", { name: modelData.name, crumb: modelData.name })
                        else
                            page.owner.open("genre", { value: modelData.name, crumb: modelData.name })
                    }
                    RowLayout {
                        width: parent.width
                        spacing: Theme.px(12)
                        Text {
                            Layout.alignment: Qt.AlignBaseline
                            text: moreRow.modelData.kind === "director" ? "more by" : "more in"
                            color: Theme.textDim
                            font.pixelSize: Theme.fontSizeSmall
                            font.letterSpacing: 1
                        }
                        LinkText {
                            Layout.alignment: Qt.AlignBaseline
                            text: moreRow.modelData.name
                            font.weight: Theme.weightBold
                            onActivated: moreRow.openAll()
                        }
                        Text {
                            Layout.alignment: Qt.AlignBaseline
                            text: Format.plural(moreRow.modelData.others, "other movie")
                            color: Theme.textDim
                            font.pixelSize: Theme.fontSizeSmall
                        }
                        Item { Layout.fillWidth: true }
                        LinkText {
                            Layout.alignment: Qt.AlignBaseline
                            text: "all →"
                            font.pixelSize: Theme.fontSizeSmall
                            onActivated: moreRow.openAll()
                        }
                    }
                    Rectangle { width: parent.width; height: 1; color: Theme.hairline }
                    CardFlow {
                        width: parent.width
                        Repeater {
                            model: moreRow.modelData.movies
                            CoverCard {
                                required property var modelData
                                width: parent.cardWidth
                                poster: true
                                placeholder: "movies"
                                source: Library.thumbnail(modelData.coverEntry)
                                online: modelData.online
                                title: modelData.values[0]
                                subtitle: [Number(modelData.values[1]) > 0 ? modelData.values[1] : "",
                                           Format.resolution(modelData.maxHeight) + (modelData.hdr ? " HDR" : "")].filter(s => s).join(" · ")
                                onActivated: page.owner.open("movie", { title: modelData.values[0], year: Number(modelData.values[1]),
                                                                         coverEntry: modelData.coverEntry, crumb: modelData.values[0] })
                            }
                        }
                    }
                }
            }
            Item { width: 1; height: Theme.px(18) }
        }
    }
}
