import QtQuick
import QtQuick.Layouts
import Tyli
import "Format.js" as Format

// A music category's home: every album, in the shared sort.
ColumnLayout {
    id: page
    objectName: "musicView"

    property var owner
    property var route: ({ page: "home", params: {} })  // this page's own: set by CategoryView
    property int depth: 0
    property string sortKey: Library.preference("sort/music", "added")
    property bool addedDescending: Library.preference("sort/music/added", true)

    spacing: 0

    BrowseHeader {
        Layout.fillWidth: true
        crumbs: page.owner.crumbs(page.depth)
        title: page.owner.category ? page.owner.category.name.toLowerCase() : ""
        subtitle: Format.plural(browser.total, "album")
        sortOptions: [{ key: "added", label: "added" }, { key: "az", label: "a–z" }, { key: "artist", label: "by artist" }, { key: "year", label: "year" }]
        sortKey: page.sortKey
        addedDescending: page.addedDescending
        onSortPicked: (key, desc) => {
            page.sortKey = key
            page.addedDescending = desc
            Library.setPreference("sort/music", key)
            Library.setPreference("sort/music/added", desc)
        }
    }
    AlbumBrowser {
        id: browser
        Layout.fillWidth: true
        Layout.fillHeight: true
        owner: page.owner
        filter: ({ category: page.owner.categoryId })
        sortKey: page.sortKey
        addedDescending: page.addedDescending
    }
}
