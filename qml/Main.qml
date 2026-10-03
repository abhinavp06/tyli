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
    property bool videoOpen: false  // the video page
    property bool fullScreen: false
    property bool expandedOpen: false   // the expanded player, over everything above the bar
    property bool expandedQueue: false  // its queue panel
    // The page in the content area, most specific first.
    readonly property string view: settingsOpen ? "settings" : searchText !== "" ? "search" : videoOpen ? "video"
                                   : queueOpen ? "queue" : homeOpen ? "home" : "category"
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
    onFullScreenChanged: visibility = fullScreen ? Window.FullScreen : Window.Windowed
    property bool sidebarCollapsed: false
    property string searchText  // what the results page shows ("" = no search)
    readonly property var currentCategory: Library.categories.find(c => c.id === currentCategoryId)

    function openSettings(page) {
        settingsOpen = true
        settings.currentPage = page
    }

    // Back and forward, like a browser: the mouse's side buttons, Alt+← / Alt+→, the
    // keyboard's Back / Forward keys. Back first leaves Settings or the search results, then
    // walks the category's pages; a page you go back to is exactly as you left it.
    function goBack() {
        if (expandedOpen)
            expandedOpen = false
        else if (settingsOpen)
            settingsOpen = false
        else if (searchText !== "")
            clearSearch()
        else if (videoOpen)
            videoOpen = false
        else if (queueOpen)
            queueOpen = false
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
        clearSearch()
    }
    function goForward() {
        if (view === "category")
            categoryView.forward()
    }

    // ---- Playing (features/player_ui.md §1)
    // Music: plays at once when nothing is loaded, otherwise asks (the queue prompt).
    // `addIds` is what "add to queue" adds: the clicked track, or the whole album.
    function playTracks(ids, start, addIds, label) {
        if (ids.length === 0)
            return
        if (Player.active)
            queuePrompt.ask(ids, start, addIds, label)
        else
            Player.playNow(ids, start)
    }
    // Video: replaces the queue (decision P1) once the picture is ready to be drawn.
    property var pendingVideo: null
    function playVideo(ids, start) {
        if (ids.length === 0)
            return
        pendingVideo = { ids: ids, start: start }
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

    ColumnLayout {
        anchors.fill: parent
        spacing: 0
        visible: Library.ready

        TopBar {
            id: topBar
            Layout.fillWidth: true
            visible: !window.fullScreen
            settingsOpen: window.settingsOpen
            onToggleSidebar: window.sidebarCollapsed = !window.sidebarCollapsed
            onSettingsRequested: window.settingsOpen ? (window.settingsOpen = false) : window.openSettings("library")
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
                onCategoryClicked: (id) => {
                    // The category already shown: back to its home page, as you left it.
                    if (id === window.currentCategoryId && window.view === "category")
                        categoryView.goToCrumb({ depth: 0 })
                    window.currentCategoryId = id
                    window.settingsOpen = false
                    window.homeOpen = false
                    window.queueOpen = false
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
                Connections { target: topBar; function onSearchTextChanged() { searchDelay.restart() } }
                SearchView {
                    anchors.fill: parent
                    visible: window.view === "search"
                    text: window.searchText
                    // Back from the opened page returns to these results (CategoryView.returnRequested).
                    onOpened: (categoryId, page, params) => {
                        const text = window.searchText
                        window.currentCategoryId = categoryId
                        window.homeOpen = false
                        window.queueOpen = false
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
                SettingsView {
                    id: settings
                    anchors.fill: parent
                    visible: window.settingsOpen
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
                window.videoOpen = false
                window.queueOpen = !window.queueOpen
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
