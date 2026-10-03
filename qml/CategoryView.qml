import QtQuick
import QtQuick.Controls
import Tyli

// A category's media, in the view that fits it (features/media_listing_ui.md §2): music
// (albums), movies, or shows. Pages open in place, with back and forward like a browser
// (the mouse's side buttons, Alt+← / Alt+→): an album, an artist, a genre, a year, a
// movie, a director, a show.
Item {
    id: view
    objectName: "categoryView"

    property var category
    signal addFolderRequested()
    signal folderSettingsRequested(int folderId)
    // Pages ask to play through here; Main decides how (the queue prompt, the video page).
    signal playRequested(var ids, int start, var addIds, string label, bool video)
    // A page opened from the search results: back from it (at `returnDepth`) asks to show them again.
    property int returnDepth: -1
    signal returnRequested()
    function play(ids, start, addIds, label, video) { playRequested(ids, start, addIds, label, video) }

    readonly property int categoryId: category ? category.id : -1
    property int kindRevision: 0
    readonly property string kind: { kindRevision; return categoryId >= 0 ? Library.categoryView(categoryId) : "none" }  // music | movies | shows | none
    readonly property var folders: category ? Library.folders.filter(f => f.categoryId === category.id) : []
    readonly property string mode: folders.length === 0 ? "empty" : kind
    // A new category, or a new kind of home page: start again from its home.
    readonly property string homeKey: categoryId + "/" + mode
    onHomeKeyChanged: home()
    Component.onCompleted: home()

    // The pages from the category's home to the one shown. Each stays alive under the next
    // (the StackView), so going back finds it exactly as it was: the same scroll position and
    // rows. `forwardRoutes` are the pages "forward" brings back, the next one last.
    property var routes: []
    property var forwardRoutes: []
    readonly property var route: routes.length > 0 ? routes[routes.length - 1] : ({ page: "home", params: {} })
    readonly property bool canGoBack: routes.length > 1
    readonly property bool canGoForward: forwardRoutes.length > 0

    function open(page, params) {
        forwardRoutes = []
        push({ page: page, params: params || {} })
    }
    function push(r) {
        routes = routes.concat([r])
        stack.pushItem(componentFor(r), { owner: view, route: r, depth: routes.length - 1 }, StackView.Immediate)
    }
    function back() {
        if (!canGoBack)
            return false
        const toSearch = returnDepth >= 0 && routes.length - 1 === returnDepth
        if (toSearch)
            returnDepth = -1
        forwardRoutes = forwardRoutes.concat([route])
        routes = routes.slice(0, routes.length - 1)
        stack.popCurrentItem(StackView.Immediate)
        if (toSearch)
            returnRequested()
        return true
    }
    function forward() {
        if (!canGoForward)
            return false
        const r = forwardRoutes[forwardRoutes.length - 1]
        forwardRoutes = forwardRoutes.slice(0, forwardRoutes.length - 1)
        push(r)
        return true
    }
    function home() {
        returnDepth = -1
        routes = [{ page: "home", params: {} }]
        forwardRoutes = []
        stack.clear(StackView.Immediate)
        stack.pushItem(componentFor(routes[0]), { owner: view, route: routes[0], depth: 0 }, StackView.Immediate)
    }

    Connections {
        target: Library
        function onCategoryViewsChanged() { view.kindRevision++ }
        // "automatic" categories can change kind once tags are read; never under an open page.
        function onContentChanged() { if (view.routes.length === 1) view.kindRevision++ }
    }

    // The breadcrumb of the page at `depth`: the category, then each page opened from it.
    function crumbs(depth) {
        const out = [{ text: category ? category.name.toLowerCase() : "", page: "home", depth: 0 }]
        for (let i = 1; i <= depth && i < routes.length; ++i)
            out.push({ text: routes[i].params.crumb || routes[i].page, depth: i })
        return out
    }
    // Like going back several pages at once: they can all come back with "forward".
    function goToCrumb(crumb) {
        if (crumb.depth < returnDepth)
            returnDepth = -1   // a crumb above the page from search: that way back is gone
        if (crumb.depth >= routes.length - 1)
            return
        forwardRoutes = forwardRoutes.concat(routes.slice(crumb.depth + 1).reverse())
        routes = routes.slice(0, crumb.depth + 1)
        stack.popToIndex(crumb.depth, StackView.Immediate)
    }

    function componentFor(r) {
        if (mode === "empty")
            return empty
        if (mode === "music") {
            switch (r.page) {
            case "album": return albumPage
            case "artist": return artistPage
            case "genre":
            case "year": return musicFilterPage
            }
            return musicHome
        }
        if (mode === "movies" || mode === "shows") {
            switch (r.page) {
            case "movie": return moviePage
            case "show": return showPage
            case "season": return seasonPage
            case "director": return personPage
            case "genre":
            case "year": return videoFilterPage
            }
            return mode === "movies" ? moviesHome : showsHome
        }
        return unsupported
    }

    StackView {
        id: stack
        anchors.fill: parent
        anchors.leftMargin: Theme.pageMargin
        anchors.rightMargin: Theme.pageMargin
        anchors.topMargin: Theme.px(20)
        anchors.bottomMargin: Theme.px(12)
        clip: true
    }

    Component {
        id: empty
        Column {
            property var owner
            property var route
            property int depth
            spacing: Theme.px(10)
            Text { text: view.category ? view.category.name.toLowerCase() : ""; color: Theme.text; font.pixelSize: Theme.fontSizeTitle; font.weight: Theme.weightBold }
            Text { text: "no folders yet"; color: Theme.textDim }
            LinkText { text: "add a folder in settings → library"; onActivated: view.addFolderRequested() }
        }
    }
    Component {
        id: unsupported
        Column {
            property var owner
            property var route
            property int depth
            spacing: Theme.px(10)
            Text { text: view.category ? view.category.name.toLowerCase() : ""; color: Theme.text; font.pixelSize: Theme.fontSizeTitle; font.weight: Theme.weightBold }
            Text { text: "books and other text get their own view later"; color: Theme.textDim }
        }
    }
    Component { id: musicHome; MusicView {} }
    Component { id: albumPage; AlbumView {} }
    Component { id: artistPage; ArtistView {} }
    Component { id: musicFilterPage; MusicFilterView {} }
    Component { id: moviesHome; VideoGridView { movies: true } }
    Component { id: showsHome; VideoGridView { movies: false } }
    Component { id: moviePage; MovieView {} }
    Component { id: showPage; ShowView {} }
    Component { id: seasonPage; SeasonView {} }
    Component { id: personPage; PersonView {} }
    Component { id: videoFilterPage; VideoGridView { movies: view.mode === "movies" } }
}
