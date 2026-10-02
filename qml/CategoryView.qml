import QtQuick
import Tyli

// A category's media, in the view that fits it (features/media_listing_ui.md §2): music
// (albums), movies, or shows. Pages open in place with a back stack: an album, an
// artist, a genre, a year, a movie, a director, a show.
Item {
    id: view
    objectName: "categoryView"

    property var category
    signal addFolderRequested()
    signal folderSettingsRequested(int folderId)

    readonly property int categoryId: category ? category.id : -1
    property string kind: "none"   // music | movies | shows | none
    property var route: ({ page: "home", params: {} })
    property var history: []

    function refreshKind() { kind = categoryId >= 0 ? Library.categoryView(categoryId) : "none" }
    function open(page, params) {
        history = history.concat([route])
        route = { page: page, params: params || {} }
    }
    function back() {
        if (history.length === 0)
            return
        route = history[history.length - 1]
        history = history.slice(0, history.length - 1)
    }
    function home() {
        history = []
        route = { page: "home", params: {} }
    }

    onCategoryIdChanged: { refreshKind(); home() }
    Component.onCompleted: refreshKind()
    Connections {
        target: Library
        function onCategoryViewsChanged() { view.refreshKind(); view.home() }
        function onContentChanged() { if (view.route.page === "home") view.refreshKind() }
    }

    readonly property var folders: category ? Library.folders.filter(f => f.categoryId === category.id) : []

    // The breadcrumb: the category, then each page opened from it.
    function crumbs() {
        const name = category ? category.name.toLowerCase() : ""
        const out = [{ text: name, page: "home", depth: 0 }]
        const all = history.concat([route])
        for (let i = 1; i < all.length; ++i)
            out.push({ text: all[i].params.crumb || all[i].page, depth: i })
        return out
    }
    function goToCrumb(crumb) {
        const all = history.concat([route])
        route = all[crumb.depth]
        history = all.slice(0, crumb.depth)
    }

    Loader {
        id: page
        anchors.fill: parent
        anchors.leftMargin: Theme.pageMargin
        anchors.rightMargin: Theme.pageMargin
        anchors.topMargin: 20
        anchors.bottomMargin: 12
        sourceComponent: {
            if (view.folders.length === 0)
                return empty
            if (view.kind === "music") {
                switch (view.route.page) {
                case "album": return albumPage
                case "artist": return artistPage
                case "genre":
                case "year": return musicFilterPage
                }
                return musicHome
            }
            if (view.kind === "movies" || view.kind === "shows") {
                switch (view.route.page) {
                case "movie": return moviePage
                case "show": return showPage
                case "director": return personPage
                case "genre":
                case "year": return videoFilterPage
                }
                return view.kind === "movies" ? moviesHome : showsHome
            }
            return unsupported
        }
    }

    Component {
        id: empty
        Column {
            spacing: 10
            Text { text: view.category ? view.category.name.toLowerCase() : ""; color: Theme.text; font.pixelSize: Theme.fontSizeTitle; font.weight: Theme.weightBold }
            Text { text: "no folders yet"; color: Theme.textDim }
            LinkText { text: "add a folder in settings → library"; onActivated: view.addFolderRequested() }
        }
    }
    Component {
        id: unsupported
        Column {
            spacing: 10
            Text { text: view.category ? view.category.name.toLowerCase() : ""; color: Theme.text; font.pixelSize: Theme.fontSizeTitle; font.weight: Theme.weightBold }
            Text { text: "books and other text get their own view later"; color: Theme.textDim }
        }
    }
    Component { id: musicHome; MusicView { owner: view } }
    Component { id: albumPage; AlbumView { owner: view } }
    Component { id: artistPage; ArtistView { owner: view } }
    Component { id: musicFilterPage; MusicFilterView { owner: view } }
    Component { id: moviesHome; VideoGridView { owner: view; movies: true } }
    Component { id: showsHome; VideoGridView { owner: view; movies: false } }
    Component { id: moviePage; MovieView { owner: view } }
    Component { id: showPage; ShowView { owner: view } }
    Component { id: personPage; PersonView { owner: view } }
    Component { id: videoFilterPage; VideoGridView { owner: view; movies: view.kind === "movies" } }
}
