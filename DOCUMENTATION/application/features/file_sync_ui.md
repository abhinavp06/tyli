# Feature: File Sync UI (v1)

| | |
|---|---|
| Status | **Shipped in `v0.1.0`** (2026-10-02): phases 1–3 done. Bridge, model, screens, and tests built; restyled by the theme (`theme.md`); tested by the user over VNC and on Windows with the GitHub Actions build. Approved 2026-10-01 |
| Branch | `abhinavp06/FILE_SCANNER_INIT` (pairs with the hoardor branch of the same name) |
| Ships in | TYLI `v0.1.0`, together with hoardor `v0.1.0` |
| hoardor feature | File Sync v1: `../hoardor/DOCUMENTATION/application/features/file_sync.md` |

## 1. Goal

A working UI over hoardor's File Sync v1, so the user can test it on real drives on Windows:
- configure categories and folders
- press **Sync** in a section and watch progress
- see what each folder holds, and what the last sync did
- deal with offline drives, held removals, and unreadable folders
- edit every hoardor setting

**Not in this feature:**
- **Listing media by category** across folders, with sorting: next feature, `media_listing`, which needs a new hoardor API first. Here the file list shows one folder at a time, using hoardor's existing `Library::entries(root)`.
- Playback, metadata (artist, album, cover art), search.
- Visual polish. The look is functional (Fusion style).

## 2. Screens

> **Superseded on 2026-10-02:** the screens were rebuilt with the theme and the new layout (`features/theme.md` §4a, §5a): folders, sync, and reports moved into Settings → Library; the category view lists the category's files (`CategoryFilesModel`, which replaced `FolderFilesModel`). The bridge API below still holds, plus `folderReports`. The wireframes below are kept for history.

### 2.1 Main window

One tab per category, in hoardor's order, plus **Settings** at the end. Categories added in Settings appear as new tabs right away.

```
┌──────────────────────────────────────────────────────────────────────────────────┐
│  Music │ Movies │ Shows │ Books │                                      ⚙ Settings │
├──────────────────────────────────────────────────────────────────────────────────┤
│                         (the selected tab's view)                                 │
└──────────────────────────────────────────────────────────────────────────────────┘
```

### 2.2 Section view (one per category tab)

```
┌──────────────────────────────────────────────────────────────────────────────────┐
│ Music                                      12,408 files · 2 folders   [ Sync ⟳ ] │
│ ▓▓▓▓▓▓▓▓▓▓▓░░░░░░░░  Syncing "Music (E:)" · folder 1 of 2 · 5,120 files  [Cancel]│  ← only while syncing
├───────────────────────────┬──────────────────────────────────────────────────────┤
│ FOLDERS                   │ FILES IN "Music (E:)"                                 │
│ ● Music (E:)       online │ Artist A/Album 1/01 Intro.flac     34.2 MB   audio    │
│   E:/Media/Music          │ Artist A/Album 1/cover.jpg          1.1 MB   image    │
│   8,201 files · synced 2m │ Artist A/Album 2/01 Song.mp3        8.0 MB   audio  ⧗ │  ← ⧗ = unsettled
│ ○ Music (F:)      offline │ …                                    (loads as you scroll)│
│   F:/Music                │                                                       │
│   4,207 files · synced 3d │                                                       │
├───────────────────────────┴──────────────────────────────────────────────────────┤
│ Last sync: Music (E:) +120 added · 3 modified · 0 removed · 2 errors  [Details]   │
│ ⚠ Music (F:): 340 files are missing (more than 25%). Removals are on hold.        │
│                                     [Show folder]  [Remove them]  [Keep for now]  │
└──────────────────────────────────────────────────────────────────────────────────┘
```

- **Folder list:**
  - shows each of this category's folders: name, path, online/offline, file count, and last sync time
  - clicking a folder shows its files
  - offline folders are greyed out, and their files stay visible (nothing touches the drive)
- **File list:** relative path, size, kind, and an "unsettled" mark. It's paged from hoardor as you scroll.
- **Sync** syncs this category in the background.
  - **While running:** the progress bar and Cancel appear, and the button is disabled.
  - **When done:** the summary bar updates, and the folder and file lists refresh.
- **Details** opens a dialog listing each folder's report and its scan errors (unreadable folders and files).
- **The held-removals banner** appears when the mass-removal guard held back removals. "Remove them" calls `apply_held_removals`.

### 2.3 Settings

```
┌──────────────────────────────────────────────────────────────────────────────────┐
│ Folders │ Categories │ Scanning │ Sync                                            │
├──────────────────────────────────────────────────────────────────────────────────┤
│ FOLDERS                                                         [ + Add folder ] │
│ Music (E:)   Music ▾   E:/Media/Music   marker ✓  case-sensitive ✗   [Relocate] [Remove] │
│ Films        Movies ▾  F:/Films         marker ✓  case-sensitive ✗   [Relocate] [Remove] │
└──────────────────────────────────────────────────────────────────────────────────┘
```

| Sub-tab | What it edits |
|---|---|
| **Folders** | Add (a folder picker, then pick a category and an optional name, with "write a .hoardor-root marker" ticked by default), change category, case-sensitive toggle, relocate (folder picker), remove (with confirmation: "entries are removed from the library; files on disk are untouched") |
| **Categories** | Add, rename, choose accepted kinds (audio, video, text, image, subtitle), delete (refused while folders belong to it, with the reason shown) |
| **Scanning** | The extension → kind table (add, change, remove rows; "Reset to defaults"), ignored names, ignored prefixes |
| **Sync** | Sync at startup, settle window, mass-removal threshold %, batch rows and milliseconds, progress interval, relocation sample size and match % |

Every field shows hoardor's default. Changes apply when you press **Save**, and **Reset to defaults** is per section.

## 3. Bridge (C++, `src/`)

Two classes:

```cpp
namespace tyli {

// One per app, exposed to QML as the singleton `Library`.
class LibraryBridge : public QObject {
    Q_OBJECT
    QML_NAMED_ELEMENT(Library)
    QML_SINGLETON
    Q_PROPERTY(QVariantList categories READ categories NOTIFY categoriesChanged)  // {id, name, kinds[]}
    Q_PROPERTY(QVariantList folders READ folders NOTIFY foldersChanged)          // {id, categoryId, name, path, online,
                                                                                //  useMarker, caseSensitive, fileCount,
                                                                                //  lastSync, heldRemovals}
    Q_PROPERTY(QVariantMap settings READ settings NOTIFY settingsChanged)        // every file::Settings field
    Q_PROPERTY(bool syncing READ syncing NOTIFY syncingChanged)
    Q_PROPERTY(QVariantMap syncProgress READ syncProgress NOTIFY syncProgressChanged)  // {folderIndex, folderCount, folderName, files}
    Q_PROPERTY(QVariantMap lastReport READ lastReport NOTIFY lastReportChanged)
public:
    Q_INVOKABLE void requestSync(int categoryId);              // -1 = every category
    Q_INVOKABLE void cancelSync();
    Q_INVOKABLE void addFolder(int categoryId, const QUrl& folder, const QString& name, bool useMarker);  // off the UI thread
    Q_INVOKABLE void relocateFolder(int folderId, const QUrl& folder);                                   // off the UI thread
    Q_INVOKABLE bool removeFolder(int folderId);
    Q_INVOKABLE bool setFolderCategory(int folderId, int categoryId);
    Q_INVOKABLE bool setFolderCaseSensitive(int folderId, bool caseSensitive);
    Q_INVOKABLE bool addCategory(const QString& name, const QStringList& kinds);
    Q_INVOKABLE bool updateCategory(int id, const QString& name, const QStringList& kinds);
    Q_INVOKABLE bool removeCategory(int id);
    Q_INVOKABLE bool saveSettings(const QVariantMap& settings);
    Q_INVOKABLE QVariantMap defaultSettings() const;
    Q_INVOKABLE QVariantList scanErrors(int folderId) const;
    Q_INVOKABLE bool applyHeldRemovals(int folderId);
signals:
    void categoriesChanged(); void foldersChanged(); void settingsChanged();
    void syncingChanged(); void syncProgressChanged(); void lastReportChanged();
    void syncFinished(QVariantMap report);
    void folderOperationFinished(bool ok, QString message);   // addFolder / relocateFolder results
    void errorOccurred(QString message);                      // anything else that failed, in plain words
};

// The files of one folder, paged from hoardor (Library::entries) as the view scrolls.
class FolderFilesModel : public QAbstractListModel {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(int folderId READ folderId WRITE setFolderId NOTIFY folderIdChanged)
    // roles: relativePath, size, kind, unsettled, mtime
    // canFetchMore / fetchMore: the next page of 200; reload() after a sync
};

}
```

**As built (2026-10-02), additions to the sketch above:**
- `ready`, `openError`, `databasePath` (constant): the error screen when the database can't be opened.
- `settingLimits` (hoardor's `file::limits` table as `{name: {min, max}}`) and `fileKinds`: QML takes ranges and kind names from hoardor instead of repeating them.
- `categoryReports`: each folder's latest report grouped by the folder's current category (`{added, modified, removed, errors, cancelled, finishedAt, folders}`), which the section's summary bar shows. Reports are kept for the session only.
- `folderOperationRunning`: drives the busy indicator while `addFolder` / `relocateFolder` run.
- `requestSync` returns `bool` (false = already queued or running). `addFolder` / `relocateFolder` take a `QString` that is either a `file://` URL (folder dialog) or a typed path. `localPath(QUrl)` turns a dialog's URL into a path to show.
- `FolderFilesModel` also has a `count` property (rows loaded so far) and a `PageSize` of 200. It reads through `LibraryBridge::instance()->library()`.
- Paths go to `std::filesystem::path` through `QString::toStdU16String()`, so non-ASCII folder names work on Windows (a narrow `std::string` would be read in the ANSI code page there).

- **Connections:** `LibraryBridge` owns the UI thread's hoardor `Database` and `Library`, and a `master::SyncWorker` (its own connection, its own thread). `SyncWorker` callbacks are forwarded to the UI thread with `QMetaObject::invokeMethod(this, ..., Qt::QueuedConnection)`.
- **Drive-touching operations** (`addFolder`, `relocateFolder`): run with `QThreadPool` on a short-lived `Database` + `Library`, and finish with `folderOperationFinished`.
- **Database location:** see ARCHITECTURE §3 (`TYLI_DATA_DIR` overrides it).

## 4. Edge cases

| Case | Behavior |
|---|---|
| Sync pressed while a sync runs | The button is disabled. A second request for the same scope would be ignored by `SyncWorker` anyway |
| Sync of another category while one runs | Queued. The progress line shows the running one |
| A folder's drive unplugged | The folder shows "offline" (greyed out). Its files stay listed. Sync reports it as offline, with hoardor's reason |
| Drive came back under a new letter | Sync reports "relocated", and the path in the folder list updates |
| Mass removal held | Banner with "Remove them", "Keep for now", and "Show folder" |
| Unreadable folders or files | The error count in the summary. Details lists them |
| Cancel | The progress bar disappears. The summary says "Cancelled — nothing was removed" |
| App closed mid-sync | `SyncWorker`'s destructor stops and joins. The window closes promptly |
| Adding an overlapping folder, a folder already in the library, a missing folder | hoardor's message in a dialog (`Overlap`, `AlreadyExists`, `NotFound`) |
| Adding a read-only folder | Added. A note says no marker could be written |
| Adding a folder on a sleeping HDD | No freeze: it runs off the UI thread with a busy indicator |
| Deleting a category that has folders | Refused, with the reason |
| Invalid setting value (e.g. threshold 150) | The field rejects it (range validation matching hoardor's limits) |
| A huge folder (500k files) | The file list pages as you scroll. Memory stays flat |
| A sync finishes while a file list is open | The list reloads, keeping the scroll near the top |
| Database can't be opened (permissions, disk full) | An error screen with the path and hoardor's message, instead of a crash |
| Sync at startup on | Starts in the background after the window appears. The UI is usable right away |

## 5. Tests (`tests/`, Qt Test, run by `ctest`)

- **`LibraryBridge`** (temporary `TYLI_DATA_DIR`, a temporary media tree):
  - default categories are exposed
  - add a folder: the signal fires, the folder appears, and it's online
  - `requestSync`: `syncing` goes true then false, `syncFinished` fires, the report counts are correct
  - cancel
  - an offline folder
  - held removals, then apply
  - categories CRUD and errors
  - settings round trip and validation
  - scan errors
- **`FolderFilesModel`:**
  - pages of 200 through 1,000 files with no gaps or duplicates
  - `reload` after a sync
  - switching folders
- **QML smoke test:**
  - load `Main.qml` offscreen with a seeded library
  - visit every tab and every Settings sub-tab
  - fail on any QML warning
  - save a screenshot of each view to `build/screenshots/` for review

## 6. Build structure (built 2026-10-02)

- **`CMakeLists.txt` gets a static library `tyli_ui`** that is the QML module (`qt_add_qml_module(tyli_ui URI Tyli ...)`). It holds the bridge sources (`src/library_bridge.*`, `src/folder_files_model.*`) and every QML file (`Main`, `SectionView`, `SyncDetailsDialog`, `SettingsView`, `FolderSettings`, `CategorySettings`, `ScanningSettings`, `SyncSettings`, `Format.js`).
- **Both the app (`tyli`) and the tests link `tyli_ui` and `tyli_uiplugin`,** so the tests load the real screens. That needs `Q_IMPORT_QML_PLUGIN(TyliPlugin)` in `main.cpp` and in the tests' main.
- **`Qt6::Concurrent`** is added for the folder operations that touch a drive. `Qt6::Test` is added for the tests. `TYLI_BUILD_TESTS` (default ON when TYLI is top-level) controls `tests/`.
- **`LibraryBridge` is a `QML_SINGLETON`** whose static `create()` returns the instance `main()` constructed. `main()` controls its lifetime and data folder, and the instance has C++ ownership.
- **Sync state:**
  - `syncing` turns true when `requestSync` is accepted.
  - While syncing, a short timer polls `SyncWorker::idle()`, because the "finished" callback runs before the worker clears its "running" flag.
  - Reports are kept per folder (`folderReports`).
- **QML module output** goes to `build/qml/Tyli` (`OUTPUT_DIRECTORY`), as Qt's tooling expects.
- **Screens as built:**
  - The category tabs are a `TabBar`; **Settings** is a separate checkable button at the right, and the sections and Settings are sibling views. That keeps Settings' position fixed when categories are added.
  - Folder pickers are a text field plus **Browse…** (Qt Quick `FolderDialog`), so a path can also be typed.
  - The "still being copied" mark is the word "settling" (the `⧗` glyph is missing from common fonts).
  - The file list is in database order (scan order, by entry id). Sorting arrives with media listing.
  - "Keep for now" on the held-removals banner hides it for the session; the removals stay held in hoardor.
- **Settings:** `saveSettings` passes the map to hoardor's `save_settings`, which validates it (`file::validate`), and shows hoardor's message on refusal. The ranges aren't duplicated in QML.

## 7. Phases (on this branch)

1. **Bridge + tests:** `LibraryBridge`, `FolderFilesModel`, a `TYLI_DATA_DIR` data folder. **Built 2026-10-02.**
2. **QML screens:** main window, section view, details dialog, Settings (4 sub-tabs), plus the smoke test with screenshots. **Built 2026-10-02**; the user is testing it over VNC.
3. **Windows check by the user:** build on Windows and run against the real external drives. Also measure a cold-HDD sync and try playback stutter (hoardor OI-1) once playback exists. **Done 2026-10-02** with the GitHub Actions build (`tyli-windows`): the user's verdict was "it's perfect"; a real sync finished too fast to see the progress line. The cold-HDD measurement and the playback-stutter check wait for the player.

Then both branches merge (hoardor `v0.1.0`, TYLI `v0.1.0`), and **media listing** starts on new branches.
