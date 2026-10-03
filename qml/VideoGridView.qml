import QtQuick
import QtQuick.Layouts
import Tyli
import "Format.js" as Format

// Movies (one card per title and year, whatever the copies) or shows, as posters. Also
// serves the genre and year pages (route.page "genre" / "year").
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

    spacing: 0

    GroupModel {
        id: groups
        engine: "video"
        groupBy: page.movies ? ["title", "year"] : ["show"]
        filter: {
            const f = { category: page.owner.categoryId, type: page.movies ? 1 : 2 }
            if (page.field)
                f[page.field] = page.value
            return f
        }
        order: page.sortKey === "added" ? "added" : page.sortKey === "year" ? "year" : "values"
        descending: page.sortKey === "added" ? page.addedDescending : page.sortKey === "year"
    }

    BrowseHeader {
        Layout.fillWidth: true
        crumbs: page.owner.crumbs(page.depth)
        label: page.field
        title: page.field ? String(page.value) : (page.owner.category ? page.owner.category.name.toLowerCase() : "")
        subtitle: Format.plural(groups.total, page.movies ? "movie" : "show")
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
        model: groups
        poster: true
        placeholder: page.movies ? "movies" : "shows"
        titleOf: (row) => row.values[0]
        subtitleOf: (row) => page.movies
            ? [row.values[1] > 0 ? row.values[1] : "", Format.resolution(row.maxHeight) + (row.hdr ? " HDR" : "")].filter(s => s).join(" · ")
            : [row.yearMin > 0 ? row.yearMin : "", Format.plural(row.count, "episode")].filter(s => s).join(" · ")
        onOpened: (row) => page.movies
            ? page.owner.open("movie", { title: row.values[0], year: Number(row.values[1]), coverEntry: row.coverEntry, crumb: row.values[0] })
            : page.owner.open("show", { show: row.values[0], coverEntry: row.coverEntry, crumb: row.values[0] })
    }
}
