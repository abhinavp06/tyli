import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Tyli

// The window: top bar, then the sidebar and either a category's media or Settings.
// A status line at the bottom shows results and errors.
ApplicationWindow {
    id: window
    width: 1280
    height: 800
    visible: true
    title: "TYLI"
    color: Theme.background
    font.family: Theme.fontFamily
    font.pixelSize: Theme.fontSize

    // Anything left unstyled (tooltips, the folder dialog on Linux) is dark too.
    palette {
        window: Theme.background
        windowText: Theme.text
        base: Theme.background
        alternateBase: Theme.background
        text: Theme.text
        button: Theme.background
        buttonText: Theme.text
        brightText: Theme.accent
        highlight: Theme.selection
        highlightedText: Theme.accent
        placeholderText: Theme.textFaint
        toolTipBase: Theme.background
        toolTipText: Theme.text
        light: Theme.hairline
        midlight: Theme.hairline
        mid: Theme.textFaint
        dark: Theme.hairline
        shadow: Theme.background
        link: Theme.accent
    }

    property int currentCategoryId: Library.categories.length > 0 ? Library.categories[0].id : -1
    property bool settingsOpen: false
    property bool homeOpen: false   // the home page (tyli_), instead of a category
    property bool queueOpen: false  // the queue page (from the now-playing bar)
    property var playlistOpen: 0    // a playlist's page (its id), Liked Songs (-1), or none (0)
    property bool videoOpen: false  // the video page
    property bool fullScreen: false
    property bool expandedOpen: false   // the expanded player, over everything above the bar
    property bool expandedQueue: false  // its queue panel
    // The page in the content area, most specific first.
    // Search works from every page, Settings included: its results cover the page, and clearing
    // them (or going back) shows that page again.
    readonly property string view: searchText !== "" ? "search" : settingsOpen ? "settings" : videoOpen ? "video"
                                   : queueOpen ? "queue" : playlistOpen !== 0 ? "playlist" : homeOpen ? "home" : "category"
    property string lastView: "category"
    // Leaving the video page pauses the video (features/player_ui.md, decision P3); the bar keeps it.
    onViewChanged: {
        if (lastView === "video" && view !== "video") {
            fullScreen = false
            if (Player.hasVideo)
                Player.pause()
        }
        lastView = view
    }
    // Full screen follows the window both ways: our button sets it, and a window that changes by
    // itself (the OS, a shortcut, minimizing) updates `fullScreen`, so its icon is always right.
    // Leaving full screen goes back to how the window was (maximized stays maximized).
    property int visibilityBeforeFullScreen: Window.Windowed
    onFullScreenChanged: {
        if (fullScreen && visibility !== Window.FullScreen) {
            visibilityBeforeFullScreen = visibility === Window.Maximized ? Window.Maximized : Window.Windowed
            visibility = Window.FullScreen
        } else if (!fullScreen && visibility === Window.FullScreen) {
            visibility = visibilityBeforeFullScreen
        }
    }
    onVisibilityChanged: (shown) => {
        if (shown === Window.FullScreen)
            fullScreen = true
        else if (shown !== Window.Hidden && shown !== Window.Minimized)
            fullScreen = false
    }
    property bool sidebarCollapsed: false
    property string searchText  // what the results page shows ("" = no search)
    readonly property var currentCategory: Library.categories.find(c => c.id === currentCategoryId)

    function openSettings(page) {
        settingsOpen = true
        settings.currentPage = page
        clearSearch()
    }

    // Back and forward, like a browser: the mouse's side buttons, Alt+← / Alt+→, the
    // keyboard's Back / Forward keys. Back first leaves the search results, then Settings, then
    // walks the category's pages; a page you go back to is exactly as you left it.
    function goBack() {
        if (expandedOpen)
            expandedOpen = false
        else if (searchText !== "")
            clearSearch()
        else if (settingsOpen)
            settingsOpen = false
        else if (videoOpen)
            videoOpen = false
        else if (queueOpen)
            queueOpen = false
        else if (playlistOpen !== 0)
            playlistOpen = 0
        else if (homeOpen)
            homeOpen = false
        else
            categoryView.back()
    }
    function openHome() {
        homeOpen = true
        settingsOpen = false
        videoOpen = false
        queueOpen = false
        playlistOpen = 0
        clearSearch()
        returnSearch = ""
    }
    function goForward() {
        if (view === "category")
            categoryView.forward()
    }

    // ---- Playing (features/player_ui.md §1)
    // Music: plays at once when nothing is loaded, otherwise asks (the queue prompt).
    // `addIds` is what "add to queue" adds: the clicked track, or the whole album.
    // `source`: what this was started from ({kind: "playlist", id} or {kind: "liked"}), for the
    // sidebar's speaker; anything else plays without one.
    function playTracks(ids, start, addIds, label, source) {
        if (ids.length === 0)
            return
        if (Player.active)
            queuePrompt.ask(ids, start, addIds, label, source || {})
        else
            Player.playNow(ids, start, source || {})
    }
    // A track's (or the bar's) playlist icon: the add menu, under `anchor` (playlists_ui.md §1.4).
    function addToPlaylist(entryIds, anchor, label) {
        if (entryIds.length > 0)
            playlistMenu.openFor(entryIds, anchor, label)
    }
    function openPlaylist(id) {
        clearSearch()
        returnSearch = ""
        settingsOpen = false
        videoOpen = false
        queueOpen = false
        homeOpen = false
        playlistOpen = id
    }
    // A link from a playlist's row (its artist or album) opens that page in its category.
    function openInCategory(categoryId, page, params) {
        playlistOpen = 0
        homeOpen = false
        queueOpen = false
        if (categoryId >= 0)
            currentCategoryId = categoryId
        categoryView.open(page, params)
    }
    // Video: replaces the queue (decision P1) once the picture is ready to be drawn.
    property var pendingVideo: null
    function playVideo(ids, start) {
        if (ids.length === 0)
            return
        pendingVideo = { ids: ids, start: start }
        clearSearch()
        settingsOpen = false
        queueOpen = false
        homeOpen = false
        videoOpen = true
        startPendingVideo()
    }
    function startPendingVideo() {
        if (!pendingVideo || !videoPage.videoReady)
            return
        Player.playNow(pendingVideo.ids, pendingVideo.start)
        pendingVideo = null
    }
    // A link from the bar or the expanded player: its page (an album, an artist), collapsing the player.
    function openLink(link) {
        expandedOpen = false
        if (!link || !link.page)
            return
        // A video's title goes back to its page (it's paused there), not to the movie's page.
        if (Player.hasVideo && link.page !== "artist")
            return openPlaying()
        settingsOpen = false
        clearSearch()
        queueOpen = false
        playlistOpen = 0
        homeOpen = false
        videoOpen = false
        currentCategoryId = link.categoryId
        categoryView.open(link.page, link.params)
    }
    // The bar's title: a video goes back to its page, music opens its album.
    function openPlaying() {
        expandedOpen = false
        if (Player.hasVideo) {
            settingsOpen = false
            clearSearch()
            videoOpen = true
            return
        }
        const link = Player.link
        if (!link || !link.page)
            return
        settingsOpen = false
        clearSearch()
        queueOpen = false
        playlistOpen = 0
        homeOpen = false
        currentCategoryId = link.categoryId
        categoryView.open(link.page, link.params)
    }
    // The search a result was opened from: back from that page shows these results again.
    property string returnSearch: ""
    function reopenSearch() {
        if (returnSearch === "")
            return
        topBar.searchText = returnSearch
        searchDelay.stop()
        searchText = returnSearch   // at once, not after the typing delay
        returnSearch = ""
    }
    function clearSearch() {
        topBar.searchText = ""
        searchText = ""
    }
    // On the window's root item, so it gets every side-button click that no control takes
    // (none do: controls and lists only take the left button).
    TapHandler {
        acceptedButtons: Qt.BackButton | Qt.ForwardButton
        onTapped: (eventPoint, button) => button === Qt.BackButton ? window.goBack() : window.goForward()
    }
    Shortcut { sequences: ["Alt+Left", "Back"]; onActivated: window.goBack() }
    Shortcut { sequences: ["Alt+Right", "Forward"]; onActivated: window.goForward() }

    // A category that was deleted: fall back to the first one.
    Connections {
        target: Library
        function onCategoriesChanged() {
            if (!window.currentCategory)
                window.currentCategoryId = Library.categories.length > 0 ? Library.categories[0].id : -1
        }
    }

    QueuePrompt { id: queuePrompt }
    PlaylistMenu { id: playlistMenu }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0
        visible: Library.ready

        TopBar {
            id: topBar
            Layout.fillWidth: true
            visible: !window.fullScreen
            settingsOpen: window.view === "settings"
            onToggleSidebar: window.sidebarCollapsed = !window.sidebarCollapsed
            // The gear: closes Settings when they're showing; over search results it shows them.
            onSettingsRequested: {
                if (window.view === "settings")
                    window.settingsOpen = false
                else if (window.settingsOpen)
                    window.clearSearch()
                else
                    window.openSettings("library")
            }
            onSyncStatusClicked: window.openSettings("library")
            onHomeRequested: window.openHome()
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            Sidebar {
                Layout.fillHeight: true
                visible: !window.fullScreen
                collapsed: window.sidebarCollapsed
                currentCategoryId: window.view === "category" ? window.currentCategoryId : -1
                currentPlaylist: window.view === "playlist" ? window.playlistOpen : 0
                onPlaylistClicked: (id) => window.openPlaylist(id)
                onCategoryClicked: (id) => {
                    // The category already shown: back to its home page, as you left it.
                    if (id === window.currentCategoryId && window.view === "category")
                        categoryView.goToCrumb({ depth: 0 })
                    window.currentCategoryId = id
                    window.clearSearch()  // results cover every page: a section ends the search
                    window.returnSearch = ""
                    window.settingsOpen = false
                    window.homeOpen = false
                    window.queueOpen = false
                    window.playlistOpen = 0
                    window.videoOpen = false
                }
            }

            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true
                // The video page stays drawn under the other pages while a video is loaded: mpv
                // waits for every frame to be drawn, and an undrawn picture stalls it (a pause took
                // seconds). A black cover hides it; it only takes input while it's the page shown.
                VideoPage {
                    id: videoPage
                    anchors.fill: parent
                    visible: window.view === "video" || Player.hasVideo || window.pendingVideo !== null
                    enabled: window.view === "video"
                    fullScreen: window.fullScreen
                    onBack: window.videoOpen = false
                    onFullScreenToggled: window.fullScreen = !window.fullScreen
                    onVideoReadyChanged: window.startPendingVideo()
                }
                Rectangle {
                    anchors.fill: parent
                    visible: videoPage.visible && window.view !== "video"
                    color: Theme.background
                }
                // Search results while the top bar's field has text (debounced: hoardor runs a
                // query per keystroke otherwise).
                Timer { id: searchDelay; interval: 400; onTriggered: window.searchText = topBar.searchText.trim() }
                Connections {
                    target: topBar
                    function onSearchTextChanged() {
                        if (topBar.searchText.trim() === "") {  // cleared: the page comes back at once
                            searchDelay.stop()
                            window.searchText = ""
                        } else {
                            searchDelay.restart()
                        }
                    }
                }
                SearchView {
                    anchors.fill: parent
                    visible: window.view === "search"
                    text: window.searchText
                    // Back from the opened page returns to these results (CategoryView.returnRequested).
                    onOpened: (categoryId, page, params) => {
                        const text = window.searchText
                        window.currentCategoryId = categoryId
                        window.settingsOpen = false
                        window.homeOpen = false
                        window.queueOpen = false
                        window.playlistOpen = 0
                        window.videoOpen = false
                        window.clearSearch()
                        categoryView.open(page, params)
                        categoryView.returnDepth = categoryView.routes.length - 1
                        window.returnSearch = text
                    }
                }
                CategoryView {
                    id: categoryView
                    anchors.fill: parent
                    visible: window.view === "category"
                    category: window.currentCategory
                    onPlayRequested: (ids, start, addIds, label, video) => video ? window.playVideo(ids, start)
                                                                                 : window.playTracks(ids, start, addIds, label)
                    onReturnRequested: window.reopenSearch()
                    onAddFolderRequested: window.openSettings("library")
                    // The drive link on an album, movie, or show: that folder's category in Settings → Library.
                    onFolderSettingsRequested: (folderId) => {
                        const folder = Library.folders.find(f => f.id === folderId)
                        window.openSettings("library")
                        if (folder)
                            settings.showCategory(folder.categoryId)
                    }
                }
                // The home page: blank for now. It gets designed after the player and playlists,
                // and is where recommendations will live (the user, 2026-10-03).
                Item {
                    objectName: "homeView"
                    anchors.fill: parent
                    visible: window.view === "home"
                }
                QueuePage {
                    anchors.fill: parent
                    visible: window.view === "queue"
                }
                PlaylistPage {
                    // The same margins as the queue page's (a top-level page has no crumbs).
                    anchors.fill: parent
                    anchors.leftMargin: Theme.pageMargin
                    anchors.rightMargin: Theme.pageMargin
                    anchors.topMargin: Theme.px(20)
                    anchors.bottomMargin: Theme.px(12)
                    visible: window.view === "playlist"
                    playlistId: window.playlistOpen
                    onPlayRequested: (ids, start, addIds, label, source) => window.playTracks(ids, start, addIds, label, source)
                    onOpenPage: (categoryId, pageName, params) => window.openInCategory(categoryId, pageName, params)
                    onDeleted: window.playlistOpen = 0
                }
                SettingsView {
                    id: settings
                    anchors.fill: parent
                    visible: window.view === "settings"
                    focusedCategoryId: window.currentCategoryId
                }
            }
        }

        PlayerBar {
            id: playerBar
            Layout.fillWidth: true
            visible: Player.active && !window.fullScreen
            queueOpen: window.expandedOpen ? window.expandedQueue : window.view === "queue"
            expanded: window.expandedOpen
            onOpenLink: (link) => window.openLink(link)
            onExpandToggled: window.expandedOpen = !window.expandedOpen
            // Expanded: the queue opens as the panel on the right; otherwise the queue page.
            onQueueToggled: {
                if (window.expandedOpen) {
                    window.expandedQueue = !window.expandedQueue
                    return
                }
                // Shown: close it. Otherwise open it over whatever covers it (results, Settings).
                if (window.view === "queue") {
                    window.queueOpen = false
                    return
                }
                window.clearSearch()
                window.settingsOpen = false
                window.videoOpen = false
                window.queueOpen = true
            }
        }

        StatusLine {
            Layout.fillWidth: true
        }
    }

    // The expanded player covers everything above the bar (features/player_ui.md §1.6).
    ExpandedPlayer {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: playerBar.visible ? playerBar.y : parent.height
        visible: window.expandedOpen && Player.active
        queueOpen: window.expandedQueue
        onCollapse: window.expandedOpen = false
        onQueueClosed: window.expandedQueue = false
        onOpenLink: (link) => window.openLink(link)
    }
    // Nothing loaded any more (the queue cleared or ended): nothing to expand.
    Connections {
        target: Player
        function onStatusChanged() { if (!Player.active) window.expandedOpen = false }
    }

    // Shown instead of everything else when the library database can't be opened.
    ColumnLayout {
        anchors.centerIn: parent
        width: Math.min(parent.width - Theme.px(48), Theme.px(640))
        visible: !Library.ready
        spacing: Theme.px(12)
        Text {
            text: "the library can't be opened"
            color: Theme.error
            font.pixelSize: Theme.fontSizeHeading
            font.weight: Theme.weightBold
        }
        Text {
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            color: Theme.text
            text: Library.openError
        }
        Text {
            Layout.fillWidth: true
            wrapMode: Text.WrapAnywhere
            color: Theme.textDim
            text: "database: " + Library.databasePath
        }
    }
}
