import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Tyli
import "Format.js" as Format

// One show: the poster and facts, the quality picker (the show's qualities, best first, with
// how many episodes each covers) and play (the next episode), then its seasons as cards, like
// an artist's albums (specials last). A season opens SeasonView, laid out like an album.
ColumnLayout {
    id: page
    objectName: "showView"

    property var owner
    property var route: ({ page: "home", params: {} })  // this page's own: set by CategoryView
    property int depth: 0
    readonly property var params: route.params.show !== undefined ? route.params : ({ show: "" })
    readonly property ShowState state: ShowState { show: page.params.show; categoryId: page.owner.categoryId }
    readonly property var any: state.allEpisodes.length > 0 ? state.pick(state.allEpisodes[0]) : null
    readonly property var directors: Format.unique(Format.flatten(state.allEpisodes, e => e.copies[0].item.directors))
    readonly property int firstYear: state.allEpisodes.reduce((y, e) => { const v = e.copies[0].item.year; return v > 0 && (y === 0 || v < y) ? v : y }, 0)

    function openSeason(s) {
        page.owner.open("season", { show: page.params.show, season: s, coverEntry: page.params.coverEntry || page.state.coverEntry, crumb: page.state.seasonTitle(s) })
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
        artWidth: Theme.px(147)  // as tall as an album cover (220)
        placeholder: "shows"
        source: Library.thumbnail(page.params.coverEntry || page.state.coverEntry)
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
                text: Format.plural(page.state.seasons.filter(s => s > 0).length, "season") + " · " + Format.plural(page.state.allEpisodes.length, "episode")
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
                options: page.state.qualities.map(q => ({ label: q.label, detail: q.episodes === q.total ? "all " + q.total + " episodes" : q.episodes + " of " + q.total + " episodes" }))
                currentIndex: page.state.qualityIndex
                onPicked: (index) => page.state.choose(index)
            },
            PlayButton { objectName: "playShow"; tip: "play the next episode (or resume one)"; onClicked: page.state.playNext(page.owner) }
        ]
    }

    Rectangle { Layout.fillWidth: true; height: 1; color: Theme.hairline }

    Flickable {
        Layout.fillWidth: true
        Layout.fillHeight: true
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        contentHeight: seasonColumn.implicitHeight
        ScrollBar.vertical: TyScrollBar {}
        Column {
            id: seasonColumn
            width: parent.width
            spacing: Theme.px(12)
            Text { text: "seasons"; color: Theme.textDim; font.pixelSize: Theme.fontSizeSmall; font.letterSpacing: 1 }
            CardFlow {
                objectName: "seasonCards"
                width: parent.width
                Repeater {
                    model: page.state.seasons
                    CoverCard {
                        required property int modelData
                        readonly property var eps: page.state.allEpisodes.filter(e => e.season === modelData)
                        readonly property int year: eps.reduce((y, e) => { const v = e.copies[0].item.year; return v > 0 && (y === 0 || v < y) ? v : y }, 0)
                        objectName: "season-" + modelData
                        width: parent.cardWidth
                        poster: true
                        placeholder: "shows"
                        // The show's poster: hoardor doesn't keep a season's own yet.
                        source: Library.thumbnail(page.params.coverEntry || page.state.coverEntry)
                        online: eps.some(e => e.copies[0].item.online)
                        title: page.state.seasonTitle(modelData)
                        progress: page.state.seasonProgress(modelData)
                        subtitle: [Format.plural(eps.length, "episode"), progress >= 0.999 ? "watched" : year > 0 ? String(year) : ""].filter(s => s).join(" · ")
                        onActivated: page.openSeason(modelData)
                    }
                }
            }
        }
    }
}
