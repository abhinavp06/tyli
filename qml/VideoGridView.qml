import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Tyli
import "Format.js" as Format

// Movies (one card per title and year, whatever the copies) or shows, as posters. Also
// serves the genre and year pages (route.page "genre" / "year"). On a category's own page,
// "by director" (movies) and "by genre" show sections, like music's "by artist", with the
// ones that have none last.
ColumnLayout {
    id: page
    objectName: movies ? "moviesView" : "showsView"

    property var owner
    property var route: ({ page: "home", params: {} })  // this page's own: set by CategoryView
    property int depth: 0
    property bool movies: true
    readonly property string field: route.page === "genre" || route.page === "year" ? route.page : ""
    readonly property var value: route.params.value !== undefined ? route.params.value : ""
    readonly property string prefKey: "sort/" + (movies ? "movies" : "shows")
    property string sortKey: field ? "az" : Library.preference(prefKey, "added")
    property bool addedDescending: field ? true : Library.preference(prefKey + "/added", true)
    // "director" / "genre": sections (only on the category's own page).
    readonly property string sectionField: !field && (sortKey === "director" || sortKey === "genre") ? sortKey : ""
    readonly property var baseFilter: {
        const f = { category: page.owner.categoryId, type: page.movies ? 1 : 2 }
        if (page.field)
            f[page.field] = page.value
        return f
    }
    readonly property var cardFields: page.movies ? ["title", "year"] : ["show"]

    function subtitleOf(row) {
        return page.movies
            ? [row.values[1] > 0 ? row.values[1] : "", Format.resolution(row.maxHeight) + (row.hdr ? " HDR" : "")].filter(s => s).join(" · ")
            : [row.yearMin > 0 ? row.yearMin : "", Format.plural(row.count, "episode")].filter(s => s).join(" · ")
    }
    function openRow(row) {
        if (page.movies)
            page.owner.open("movie", { title: row.values[0], year: Number(row.values[1]), coverEntry: row.coverEntry, crumb: row.values[0] })
        else
            page.owner.open("show", { show: row.values[0], coverEntry: row.coverEntry, crumb: row.values[0] })
    }
    function openSection(name) {
        if (page.sectionField === "director")
            page.owner.open("director", { name: name, crumb: name })
        else
            page.owner.open("genre", { value: name, crumb: name })
    }

    spacing: 0

    GroupModel {
        id: groups
        engine: "video"
        groupBy: page.movies ? ["title", "year"] : ["show"]
        filter: page.baseFilter
        order: page.sortKey === "added" ? "added" : page.sortKey === "year" ? "year" : "values"
        descending: page.sortKey === "added" ? page.addedDescending : page.sortKey === "year"
    }

    BrowseHeader {
        Layout.fillWidth: true
        crumbs: page.owner.crumbs(page.depth)
        label: page.field
        title: page.field ? String(page.value) : (page.owner.category ? page.owner.category.name.toLowerCase() : "")
        subtitle: Format.plural(groups.total, page.movies ? "movie" : "show")
        sortOptions: page.field ? [{ key: "added", label: "added" }, { key: "az", label: "a–z" }, { key: "year", label: "year" }]
                   : page.movies ? [{ key: "added", label: "added" }, { key: "az", label: "a–z" }, { key: "year", label: "year" },
                                    { key: "director", label: "by director" }, { key: "genre", label: "by genre" }]
                   : [{ key: "added", label: "added" }, { key: "az", label: "a–z" }, { key: "year", label: "year" }, { key: "genre", label: "by genre" }]
        sortKey: page.sortKey
        addedDescending: page.addedDescending
        onBack: page.owner.back()
        onCrumbClicked: (crumb) => page.owner.goToCrumb(crumb)
        onSortPicked: (key, desc) => {
            page.sortKey = key
            page.addedDescending = desc
            if (!page.field) {
                Library.setPreference(page.prefKey, key)
                Library.setPreference(page.prefKey + "/added", desc)
            }
        }
    }

    CoverGrid {
        objectName: "posterGrid"
        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.topMargin: Theme.px(18)
        visible: page.sectionField === ""
        model: page.sectionField === "" ? groups : null
        poster: true
        placeholder: page.movies ? "movies" : "shows"
        titleOf: (row) => row.values[0]
        subtitleOf: (row) => page.subtitleOf(row)
        onOpened: (row) => page.openRow(row)
    }

    // "by director" / "by genre": a section per name, a–z, its cards a–z; a section for the
    // ones with none ("no director") last. A section shows its first page (120) of cards;
    // its name opens the director's or genre's page with all of them.
    ListView {
        id: sections
        objectName: "videoSections"
        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.topMargin: Theme.px(12)
        visible: page.sectionField !== ""
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        spacing: Theme.px(22)
        ScrollBar.vertical: TyScrollBar {}
        model: GroupModel {
            engine: "video"
            groupBy: page.sectionField !== "" ? [page.sectionField] : []
            filter: page.baseFilter
        }
        delegate: VideoSection {
            required property var model
            width: sections.width
            name: model.values[0]
            filter: Object.assign({ [page.sectionField]: model.values[0] }, page.baseFilter)
        }
        footer: VideoSection {
            objectName: "noneSection"
            width: sections.width
            topPadding: Theme.px(22)
            visible: page.sectionField !== "" && cards.total > 0
            name: ""
            filter: page.sectionField !== "" ? Object.assign({ [page.sectionField]: null }, page.baseFilter) : page.baseFilter
        }
    }

    // One section: its name (a link) and count, a hairline, then its cards.
    component VideoSection: Column {
        id: section
        property string name           // "" = the ones with none
        property var filter
        readonly property alias cards: sectionCards
        spacing: Theme.px(12)
        Row {
            spacing: Theme.px(12)
            LinkText {
                id: sectionName
                visible: section.name !== ""
                text: section.name
                font.weight: Theme.weightBold
                onActivated: page.openSection(section.name)
            }
            Text {
                id: noneName
                visible: section.name === ""
                text: page.sectionField === "director" ? "no director" : "no genre"
                color: Theme.textDim
                font.weight: Theme.weightBold
            }
            // On the name's baseline (the name is bigger: top-aligned, the count sat high).
            Text {
                anchors.baseline: section.name !== "" ? sectionName.baseline : noneName.baseline
                text: Format.plural(sectionCards.total, page.movies ? "movie" : "show")
                color: Theme.textDim
                font.pixelSize: Theme.fontSizeSmall
            }
        }
        Rectangle { width: parent.width; height: 1; color: Theme.hairline }
        CardFlow {
            width: parent.width
            Repeater {
                model: GroupModel {
                    id: sectionCards
                    engine: "video"
                    groupBy: page.cardFields
                    filter: section.filter
                    order: "values"
                }
                CoverCard {
                    required property var model
                    width: parent.cardWidth
                    poster: true
                    placeholder: page.movies ? "movies" : "shows"
                    source: model.thumbnail
                    online: model.online
                    title: model.values[0]
                    subtitle: page.subtitleOf(model)
                    onActivated: page.openRow(model)
                }
            }
        }
    }
}
