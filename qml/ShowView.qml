import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Tyli
import "Format.js" as Format

// One show: the poster and facts, the quality picker (the show's qualities, best first,
// with how many episodes each covers), season tabs, and the season's episodes.
ColumnLayout {
    id: page
    objectName: "showView"

    property var owner
    property var route: ({ page: "home", params: {} })  // this page's own: set by CategoryView
    property int depth: 0
    readonly property var params: route.params.show !== undefined ? route.params : ({ show: "" })
    property var info: Library.showCopies(params.show, owner.categoryId)
    readonly property string qualityKey: "quality/show/" + encodeURIComponent(params.show)
    property string chosen: Library.preference(qualityKey, "")
    readonly property int qualityIndex: Math.max(0, (info.qualities || []).findIndex(q => q.label === chosen))
    readonly property string quality: info.qualities && info.qualities[qualityIndex] ? info.qualities[qualityIndex].label : ""
    readonly property var seasons: Format.unique((info.episodes || []).map(e => e.season))
    property int seasonIndex: 0
    readonly property int season: seasons.length > 0 ? seasons[Math.min(seasonIndex, seasons.length - 1)] : -1
    // Each episode in the chosen quality, or its best copy when it has no copy in that quality.
    function pick(e) {
        const exact = e.copies.find(c => c.label === page.quality)
        return (exact || e.copies[0]).item
    }
    readonly property var episodes: (info.episodes || []).filter(e => e.season === season).map(e => pick(e))
    readonly property var any: episodes.length > 0 ? episodes[0] : null
    readonly property var directors: Format.unique(Format.flatten(info.episodes || [], e => e.copies[0].item.directors))
    readonly property int firstYear: (info.episodes || []).reduce((y, e) => { const v = e.copies[0].item.year; return v > 0 && (y === 0 || v < y) ? v : y }, 0)

    Connections {
        target: Library
        function onContentChanged() { page.info = Library.showCopies(page.params.show, page.owner.categoryId) }
    }

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
        artWidth: Theme.px(130)
        placeholder: "shows"
        source: Library.thumbnail(page.params.coverEntry || 0)
        online: page.any ? page.any.online : true
        label: "show"
        title: page.params.show

        Flow {
            Layout.fillWidth: true
            spacing: Theme.px(14)
            LinkText {
                visible: page.firstYear > 0
                text: page.firstYear
                onActivated: page.owner.open("year", { value: page.firstYear, crumb: String(page.firstYear) })
            }
            Text {
                color: Theme.textDim
                text: Format.plural(page.seasons.length, "season") + " · " + Format.plural((page.info.episodes || []).length, "episode")
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
            spacing: Theme.px(6)
            visible: page.any !== null
            Icon { name: "drive"; size: Theme.px(14); anchors.verticalCenter: parent.verticalCenter }
            LinkText {
                text: page.any ? Library.folderLabel(page.any.rootId) : ""
                font.pixelSize: Theme.fontSizeSmall
                onActivated: page.owner.folderSettingsRequested(page.any.rootId)
            }
        }

        actions: [
            QualityPicker {
                objectName: "qualityPicker"
                options: (page.info.qualities || []).map(q => ({ label: q.label, detail: q.episodes === q.total ? "all " + q.total + " episodes" : q.episodes + " of " + q.total + " episodes" }))
                currentIndex: page.qualityIndex
                onPicked: (index) => {
                    page.chosen = page.info.qualities[index].label
                    Library.setPreference(page.qualityKey, page.chosen)
                }
            },
            PlayButton { tip: "play the next episode" }
        ]
    }

    Item {
        Layout.fillWidth: true
        implicitHeight: tabs.implicitHeight
        visible: page.seasons.length > 1
        Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hairline }
        Row {
            id: tabs
            Repeater {
                model: page.seasons
                TyTab {
                    required property var modelData
                    required property int index
                    text: modelData === 0 ? "specials" : modelData < 0 ? "other" : "season " + modelData
                    checked: page.seasonIndex === index
                    onClicked: page.seasonIndex = index
                }
            }
        }
    }

    ListView {
        objectName: "episodeList"
        Layout.fillWidth: true
        Layout.fillHeight: true
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        model: page.episodes
        ScrollBar.vertical: TyScrollBar {}
        header: Row {
            height: Theme.px(28)
            spacing: Theme.px(16)
            leftPadding: Theme.px(12)
            Text { width: Theme.px(28); horizontalAlignment: Text.AlignRight; text: "#"; color: Theme.textDim; font.pixelSize: Theme.fontSizeSmall }
            Text { width: Theme.px(400); text: "name"; color: Theme.textDim; font.pixelSize: Theme.fontSizeSmall }
        }
        delegate: Rectangle {
            id: row
            required property var modelData
            width: ListView.view.width
            height: Theme.px(34)
            color: area.containsMouse ? Theme.hover : "transparent"
            Row {
                anchors.verticalCenter: parent.verticalCenter
                spacing: Theme.px(16)
                leftPadding: Theme.px(12)
                Text { width: Theme.px(28); horizontalAlignment: Text.AlignRight; text: row.modelData.episode > 0 ? row.modelData.episode : ""; color: Theme.textDim }
                Text {
                    width: row.width - Theme.px(12 + 28 + 16 * 3 + 70 + 120)
                    text: row.modelData.title
                    elide: Text.ElideRight
                    color: !row.modelData.online ? Theme.textFaint : row.modelData.fromName ? Theme.textDim : Theme.text
                }
                Text { width: Theme.px(70); horizontalAlignment: Text.AlignRight; text: row.modelData.duration > 0 ? Format.runtime(row.modelData.duration) : ""; color: Theme.textDim }
                Text { width: Theme.px(120); text: Format.resolution(row.modelData.height) + (row.modelData.hdr ? " " + row.modelData.hdr : ""); color: Theme.textFaint; font.pixelSize: Theme.fontSizeSmall }
            }
            MouseArea {
                id: area
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: Library.notice("playing “" + row.modelData.title + "” comes with the player")
            }
        }
    }
}
