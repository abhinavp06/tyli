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
        if (settingsOpen)
            settingsOpen = false
        else if (searchText !== "")
            clearSearch()
        else if (homeOpen)
            homeOpen = false
        else
            categoryView.back()
    }
    function openHome() {
        homeOpen = true
        settingsOpen = false
        clearSearch()
    }
    function goForward() {
        if (!settingsOpen && !homeOpen && searchText === "")
            categoryView.forward()
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

    ColumnLayout {
        anchors.fill: parent
        spacing: 0
        visible: Library.ready

        TopBar {
            id: topBar
            Layout.fillWidth: true
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
                collapsed: window.sidebarCollapsed
                currentCategoryId: window.settingsOpen || window.homeOpen ? -1 : window.currentCategoryId
                onCategoryClicked: (id) => {
                    // The category already shown: back to its home page, as you left it.
                    if (id === window.currentCategoryId && !window.settingsOpen && !window.homeOpen && window.searchText === "")
                        categoryView.goToCrumb({ depth: 0 })
                    window.currentCategoryId = id
                    window.settingsOpen = false
                    window.homeOpen = false
                }
            }

            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true
                // Search results while the top bar's field has text (debounced: hoardor runs a
                // query per keystroke otherwise).
                Timer { id: searchDelay; interval: 200; onTriggered: window.searchText = topBar.searchText.trim() }
                Connections { target: topBar; function onSearchTextChanged() { searchDelay.restart() } }
                SearchView {
                    anchors.fill: parent
                    visible: window.searchText !== "" && !window.settingsOpen
                    text: window.searchText
                    onOpened: (categoryId, page, params) => {
                        window.currentCategoryId = categoryId
                        window.homeOpen = false
                        window.clearSearch()
                        categoryView.open(page, params)
                    }
                }
                CategoryView {
                    id: categoryView
                    anchors.fill: parent
                    visible: !window.settingsOpen && !window.homeOpen && window.searchText === ""
                    category: window.currentCategory
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
                    visible: window.homeOpen && !window.settingsOpen && window.searchText === ""
                }
                SettingsView {
                    id: settings
                    anchors.fill: parent
                    visible: window.settingsOpen
                    focusedCategoryId: window.currentCategoryId
                }
            }
        }

        StatusLine {
            Layout.fillWidth: true
        }
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
