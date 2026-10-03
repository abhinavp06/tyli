import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Tyli
import "Format.js" as Format

// One season of a show, laid out like an album page (mockup season.html): the poster and the
// season's facts (its show a link back), quality and play at the top right, then its episodes
// as tracks. An episode plays with the rest of the season after it.
ColumnLayout {
    id: page
    objectName: "seasonView"

    property var owner
    property var route: ({ page: "season", params: {} })  // this page's own: set by CategoryView
    property int depth: 0
    readonly property var params: route.params.show !== undefined ? route.params : ({ show: "", season: -1 })
    readonly property ShowState state: ShowState { show: page.params.show; categoryId: page.owner.categoryId; season: page.params.season }
    readonly property var episodes: state.picked(state.episodesHere)
    readonly property var any: episodes.length > 0 ? episodes[0] : null
    readonly property int viewedCount: episodes.filter(e => state.viewed(e)).length
    readonly property var directors: Format.unique(Format.flatten(state.episodesHere, e => e.copies[0].item.directors))
    readonly property int firstYear: state.episodesHere.reduce((y, e) => { const v = e.copies[0].item.year; return v > 0 && (y === 0 || v < y) ? v : y }, 0)

    spacing: Theme.px(16)

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
        placeholder: "shows"
        source: Library.thumbnail(page.params.coverEntry || page.state.coverEntry)
        online: page.any ? page.any.online : true
        label: "season"
        title: page.state.seasonTitle(page.params.season)

        Flow {
            Layout.fillWidth: true
            spacing: Theme.px(14)
            LinkText {
                objectName: "seasonShowLink"
                text: page.params.show
                onActivated: page.owner.back()
            }
            LinkText {
                visible: page.firstYear > 0
                text: page.firstYear
                onActivated: page.owner.open("year", { value: page.firstYear, crumb: String(page.firstYear) })
            }
            Repeater {
                model: page.any ? page.any.genres : []
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
            visible: page.directors.length > 0
            Text { text: "directed by"; color: Theme.textDim; font.pixelSize: Theme.fontSizeSmall }
            Repeater {
                model: page.directors.slice(0, 6)
                LinkText {
                    required property string modelData
                    text: modelData
                    onActivated: page.owner.open("director", { name: modelData, crumb: modelData })
                }
            }
        }
        Row {
            spacing: Theme.px(14)
            Text {
                anchors.verticalCenter: parent.verticalCenter
                color: Theme.textDim
                text: [Format.plural(page.episodes.length, "episode"), Format.runtime(page.episodes.reduce((t, e) => t + e.duration, 0)),
                       page.viewedCount > 0 ? page.viewedCount + " watched" : ""].filter(s => s).join(" · ")
            }
            Row {
                spacing: Theme.px(6)
                anchors.verticalCenter: parent.verticalCenter
                visible: page.any !== null
                Icon { name: "drive"; size: Theme.px(14); anchors.verticalCenter: parent.verticalCenter }
                LinkText {
                    text: page.any ? Library.folderLabel(page.any.rootId) : ""
                    font.pixelSize: Theme.fontSizeSmall
                    onActivated: page.owner.folderSettingsRequested(page.any.rootId)
                }
            }
        }

        actions: [
            QualityPicker {
                objectName: "qualityPicker"
                options: page.state.qualities.map(q => ({ label: q.label, detail: q.episodes === q.total ? "all " + q.total + " episodes" : q.episodes + " of " + q.total + " episodes" }))
                currentIndex: page.state.qualityIndex
                onPicked: (index) => page.state.choose(index)
            },
            PlayButton { objectName: "playSeason"; tip: "play the season (resumes the episode you're in)"; onClicked: page.state.playNext(page.owner) }
        ]
    }

    Rectangle { Layout.fillWidth: true; height: 1; color: Theme.hairline }

    ListView {
        id: episodeList
        objectName: "episodeList"
        Layout.fillWidth: true
        Layout.fillHeight: true
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        model: page.episodes
        ScrollBar.vertical: TyScrollBar {}
        // The same columns as the rows: #, name (fills), length, quality (both right-aligned, like
        // the album page's length and bit rate), then the viewed check.
        readonly property real nameWidth: width - Theme.px(12 + 28 + 16 * 4 + 70 + 120 + 22)
        header: Row {
            height: Theme.px(28)
            spacing: Theme.px(16)
            leftPadding: Theme.px(12)
            Text { width: Theme.px(28); horizontalAlignment: Text.AlignRight; text: "#"; color: Theme.textDim; font.pixelSize: Theme.fontSizeSmall }
            Text { width: episodeList.nameWidth; text: "name"; color: Theme.textDim; font.pixelSize: Theme.fontSizeSmall }
            Text { width: Theme.px(70); horizontalAlignment: Text.AlignRight; text: "length"; color: Theme.textDim; font.pixelSize: Theme.fontSizeSmall }
            Text { width: Theme.px(120); horizontalAlignment: Text.AlignRight; text: "quality"; color: Theme.textDim; font.pixelSize: Theme.fontSizeSmall }
        }
        delegate: Rectangle {
            id: row
            required property var modelData
            width: ListView.view.width
            height: Theme.px(34)
            color: area.containsMouse ? Theme.hover : "transparent"
            // Every cell centered on the row (the check's cell is full height; top-aligned text sat
            // above the centered name).
            Row {
                height: row.height
                spacing: Theme.px(16)
                leftPadding: Theme.px(12)
                Text { anchors.verticalCenter: parent.verticalCenter; width: Theme.px(28); horizontalAlignment: Text.AlignRight; text: row.modelData.episode > 0 ? row.modelData.episode : ""; color: Theme.textDim }
                Column {
                    anchors.verticalCenter: parent.verticalCenter
                    width: episodeList.nameWidth
                    spacing: Theme.px(3)
                    Text {
                        width: parent.width
                        text: row.modelData.title
                        elide: Text.ElideRight
                        color: row.modelData.entryId === Player.entryId ? Theme.accent : !row.modelData.online ? Theme.textFaint
                             : row.modelData.fromName ? Theme.textDim : Theme.text
                    }
                    // Partway: a green line under the name, as far as you got (mockup player-markers.html).
                    Rectangle {
                        objectName: "progress-" + row.modelData.entryId
                        visible: page.state.partway(row.modelData)
                        width: parent.width * 0.6
                        height: Theme.lineWidth
                        color: Theme.hairline
                        Rectangle {
                            readonly property var saved: page.state.watched[row.modelData.entryId]
                            width: parent.width * (saved && saved.duration > 0 ? Math.min(1, saved.position / saved.duration) : 0)
                            height: parent.height
                            color: Theme.accent
                        }
                    }
                }
                Text { anchors.verticalCenter: parent.verticalCenter; width: Theme.px(70); horizontalAlignment: Text.AlignRight; text: row.modelData.duration > 0 ? Format.runtime(row.modelData.duration) : ""; color: Theme.textDim }
                Text { anchors.verticalCenter: parent.verticalCenter; width: Theme.px(120); horizontalAlignment: Text.AlignRight; text: Format.resolution(row.modelData.height) + (row.modelData.hdr ? " " + row.modelData.hdr : ""); color: Theme.textFaint; font.pixelSize: Theme.fontSizeSmall }
                // Viewed: a dim green check.
                Item {
                    width: Theme.px(22)
                    height: row.height
                    Icon { objectName: "viewed-" + row.modelData.entryId; anchors.centerIn: parent; visible: page.state.viewed(row.modelData); name: "check"; size: Theme.px(14); color: Theme.accentDim }
                }
            }
            MouseArea {
                id: area
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: page.state.playFrom(page.owner, row.modelData)
            }
        }
    }
}
