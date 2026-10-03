import QtQuick
import QtQuick.Layouts
import Tyli
import "Format.js" as Format

// The albums of one genre or one year (opened from an album's links).
ColumnLayout {
    id: page
    objectName: "musicFilterView"

    property var owner
    property var route: ({ page: "home", params: {} })  // this page's own: set by CategoryView
    property int depth: 0
    readonly property string field: route.page             // "genre" | "year"
    readonly property var value: route.params.value !== undefined ? route.params.value : ""
    property string sortKey: field === "year" ? "az" : "artist"
    property bool addedDescending: true

    spacing: 0

    BrowseHeader {
        Layout.fillWidth: true
        crumbs: page.owner.crumbs(page.depth)
        label: page.field
        title: String(page.value)
        subtitle: Format.plural(browser.total, "album")
        sortOptions: [{ key: "added", label: "added" }, { key: "az", label: "a–z" }, { key: "artist", label: "by artist" }, { key: "year", label: "year" }]
        sortKey: page.sortKey
        addedDescending: page.addedDescending
        onBack: page.owner.back()
        onCrumbClicked: (crumb) => page.owner.goToCrumb(crumb)
        onSortPicked: (key, desc) => { page.sortKey = key; page.addedDescending = desc }
    }
    AlbumBrowser {
        id: browser
        Layout.fillWidth: true
        Layout.fillHeight: true
        owner: page.owner
        filter: {
            const f = { category: page.owner.categoryId }
            f[page.field] = page.value
            return f
        }
        sortKey: page.sortKey
        addedDescending: page.addedDescending
    }
}
