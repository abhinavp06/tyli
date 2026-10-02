#pragma once

#include <hoardor/db/database.hpp>
#include <hoardor/file/library.hpp>
#include <hoardor/master/sync_worker.hpp>

#include <QHash>
#include <QObject>
#include <QString>
#include <QTimer>
#include <QUrl>
#include <QVariantList>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>

#include <functional>
#include <memory>
#include <optional>

class QQmlEngine;
class QJSEngine;

namespace tyli {

// The one object QML talks to (the singleton `Library`). It owns the UI thread's
// hoardor connection and the background SyncWorker, and turns hoardor's types into
// Qt types. Reads run on the UI thread (SQLite only, never a media drive). Anything
// that can touch a drive runs elsewhere and reports back with a signal.
class LibraryBridge : public QObject {
    Q_OBJECT
    QML_NAMED_ELEMENT(Library)
    QML_SINGLETON

    Q_PROPERTY(bool ready READ ready CONSTANT)
    Q_PROPERTY(QString openError READ openError CONSTANT)
    Q_PROPERTY(QString databasePath READ databasePath CONSTANT)
    // [{id, name, kinds: ["audio", ...]}]
    Q_PROPERTY(QVariantList categories READ categories NOTIFY categoriesChanged)
    // [{id, categoryId, name, path, status: "online"|"offline"|"unknown", useMarker, caseSensitive,
    //   fileCount, lastSync (ms since the epoch, 0 = never), heldRemovals}]
    Q_PROPERTY(QVariantList folders READ folders NOTIFY foldersChanged)
    // Every file::Settings field, camelCase (see settingsToMap).
    Q_PROPERTY(QVariantMap settings READ settings NOTIFY settingsChanged)
    // {settingName: {min, max}} from hoardor's limits table, so QML never duplicates the ranges.
    Q_PROPERTY(QVariantMap settingLimits READ settingLimits CONSTANT)
    Q_PROPERTY(QStringList fileKinds READ fileKinds CONSTANT)
    Q_PROPERTY(bool syncing READ syncing NOTIFY syncingChanged)
    // {folderIndex (1-based), folderCount, folderId, folderName, files, errors}; empty when not syncing.
    Q_PROPERTY(QVariantMap syncProgress READ syncProgress NOTIFY syncProgressChanged)
    // The last finished sync: {categoryId (-1 = everything), cancelled, finishedAt, folders: [report]}.
    Q_PROPERTY(QVariantMap lastReport READ lastReport NOTIFY lastReportChanged)
    // The latest report of each category, keyed by category id (as a string): {finishedAt, cancelled,
    // added, modified, removed, errors, folders: [report]}. Kept for this session.
    Q_PROPERTY(QVariantMap categoryReports READ categoryReports NOTIFY lastReportChanged)
    Q_PROPERTY(bool folderOperationRunning READ folderOperationRunning NOTIFY folderOperationRunningChanged)

public:
    // Opens (or creates) `dataDir`/library.db and starts the SyncWorker. On failure the
    // bridge still exists, with ready = false and openError saying why.
    explicit LibraryBridge(const QString& dataDir, QObject* parent = nullptr);
    ~LibraryBridge() override;

    // TYLI_DATA_DIR if set, otherwise the app-data folder (ARCHITECTURE §3).
    static QString defaultDataDir();

    // The instance QML gets as `Library`. main() (or a test) constructs it and sets it here
    // before loading any QML, and keeps ownership.
    static void setInstance(LibraryBridge* bridge) { instance_ = bridge; }
    static LibraryBridge* instance() { return instance_; }
    static LibraryBridge* create(QQmlEngine* qmlEngine, QJSEngine* jsEngine);

    // The UI thread's hoardor library, for FolderFilesModel. Null when not ready.
    hoardor::file::Library* library() { return library_ ? &*library_ : nullptr; }

    bool ready() const { return library_.has_value(); }
    QString openError() const { return openError_; }
    QString databasePath() const { return databasePath_; }
    QVariantList categories() const { return categories_; }
    QVariantList folders() const { return folders_; }
    QVariantMap settings() const { return settings_; }
    QVariantMap settingLimits() const;
    QStringList fileKinds() const;
    bool syncing() const { return syncing_; }
    QVariantMap syncProgress() const { return syncProgress_; }
    QVariantMap lastReport() const { return lastReport_; }
    QVariantMap categoryReports() const;
    bool folderOperationRunning() const { return folderOperations_ > 0; }

    // Sync (in the background, on hoardor's SyncWorker).
    Q_INVOKABLE bool requestSync(int categoryId);  // -1 = every category; false if already queued
    Q_INVOKABLE void cancelSync();

    // Folders. addFolder and relocateFolder touch the drive, so they run off the UI thread
    // and finish with folderOperationFinished. `folder` is a file:// URL or a plain path.
    Q_INVOKABLE void addFolder(int categoryId, const QString& folder, const QString& name, bool useMarker);
    Q_INVOKABLE void relocateFolder(int folderId, const QString& folder);
    Q_INVOKABLE bool removeFolder(int folderId);
    Q_INVOKABLE bool setFolderCategory(int folderId, int categoryId);
    Q_INVOKABLE bool setFolderCaseSensitive(int folderId, bool caseSensitive);
    Q_INVOKABLE bool applyHeldRemovals(int folderId);
    // [{path, isDirectory, message}] from the folder's last sync.
    Q_INVOKABLE QVariantList scanErrors(int folderId);
    // A folder dialog's URL as a path to show (native separators).
    Q_INVOKABLE QString localPath(const QUrl& url) const;

    // Categories.
    Q_INVOKABLE bool addCategory(const QString& name, const QStringList& kinds);
    Q_INVOKABLE bool updateCategory(int id, const QString& name, const QStringList& kinds);
    Q_INVOKABLE bool removeCategory(int id);

    // Settings. saveSettings refuses invalid values with hoardor's message (errorOccurred).
    Q_INVOKABLE bool saveSettings(const QVariantMap& settings);
    Q_INVOKABLE QVariantMap defaultSettings() const;

signals:
    void categoriesChanged();
    void foldersChanged();
    void settingsChanged();
    void syncingChanged();
    void syncProgressChanged();
    void lastReportChanged();
    void folderOperationRunningChanged();
    void syncFinished(const QVariantMap& report);
    void folderOperationFinished(bool ok, const QString& message);
    // Anything else that failed, in plain words.
    void errorOccurred(const QString& message);

private:
    void open(const QString& dataDir);
    void reloadCategories();
    void reloadFolders();
    void reloadSettings();
    void setSyncing(bool syncing);
    void onProgress(const hoardor::file::SyncProgress& progress);
    void onFinished(std::optional<hoardor::file::CategoryId> scope, const hoardor::file::SyncReport& report);
    void checkIdle();
    QString folderName(qint64 folderId) const;
    // Reports a hoardor error through errorOccurred and returns false; returns true on success.
    template <class T>
    bool check(const hoardor::file::Result<T>& result);

    // Runs `work` (which opens its own connection) on Qt's thread pool and reports the
    // result through folderOperationFinished.
    void runFolderOperation(std::function<QString(hoardor::file::Library&, QString&)> work);

    inline static LibraryBridge* instance_ = nullptr;

    QString databasePath_;
    QString openError_;
    std::optional<hoardor::db::Database> database_;  // the UI thread's connection
    std::optional<hoardor::file::Library> library_;  // uses database_, so declared after it
    std::unique_ptr<hoardor::master::SyncWorker> worker_;

    QVariantList categories_;
    QVariantList folders_;
    QVariantMap settings_;
    bool syncing_ = false;
    QVariantMap syncProgress_;
    QVariantMap lastReport_;
    QHash<qint64, QVariantMap> folderReports_;  // the latest report per folder
    QTimer idleTimer_;
    int folderOperations_ = 0;
};

}
