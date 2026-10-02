#include "library_bridge.hpp"

#include <QDateTime>
#include <QDir>
#include <QFutureWatcher>
#include <QJSEngine>
#include <QStandardPaths>
#include <QUrl>
#include <QtConcurrent/QtConcurrentRun>

#include <algorithm>
#include <filesystem>
#include <map>
#include <utility>

namespace tyli {

namespace file = hoardor::file;

namespace {

QString qstr(const std::string& utf8) { return QString::fromStdString(utf8); }

// A folder from QML: a file:// URL (from a folder dialog) or a plain path (typed).
// The UTF-16 detour makes std::filesystem treat the text as Unicode on Windows too,
// where a narrow std::string would be read in the ANSI code page.
std::filesystem::path toPath(const QString& folder) {
    const QString local = folder.startsWith(QLatin1String("file:")) ? QUrl(folder).toLocalFile() : folder.trimmed();
    return std::filesystem::path(local.toStdU16String());
}

qint64 nsToMs(std::int64_t ns) { return ns / 1'000'000; }

QString statusName(file::RootStatus status) {
    switch (status) {
    case file::RootStatus::Online: return QStringLiteral("online");
    case file::RootStatus::Offline: return QStringLiteral("offline");
    case file::RootStatus::Unknown: break;
    }
    return QStringLiteral("unknown");
}

QString outcomeName(file::RootSyncOutcome outcome) {
    switch (outcome) {
    case file::RootSyncOutcome::Synced: return QStringLiteral("synced");
    case file::RootSyncOutcome::Offline: return QStringLiteral("offline");
    case file::RootSyncOutcome::Cancelled: return QStringLiteral("cancelled");
    case file::RootSyncOutcome::Failed: break;
    }
    return QStringLiteral("failed");
}

QStringList kindNames(const std::vector<file::FileKind>& kinds) {
    QStringList out;
    for (file::FileKind kind : kinds) out << qstr(std::string(file::to_string(kind)));
    return out;
}

std::optional<std::vector<file::FileKind>> parseKinds(const QStringList& names) {
    std::vector<file::FileKind> out;
    for (const QString& name : names) {
        auto kind = file::file_kind_from_string(name.toStdString());
        if (!kind) return std::nullopt;
        out.push_back(*kind);
    }
    return out;
}

QStringList toStringList(const std::vector<std::string>& values) {
    QStringList out;
    for (const auto& value : values) out << qstr(value);
    return out;
}

std::vector<std::string> fromStringList(const QStringList& values) {
    std::vector<std::string> out;
    for (const QString& value : values) {
        if (!value.trimmed().isEmpty()) out.push_back(value.trimmed().toStdString());
    }
    return out;
}

QVariantMap settingsToMap(const file::Settings& s) {
    std::map<std::string, file::FileKind> sorted(s.extension_kinds.begin(), s.extension_kinds.end());
    QVariantList kinds;
    for (const auto& [extension, kind] : sorted) {
        kinds << QVariantMap{{"extension", qstr(extension)}, {"kind", qstr(std::string(file::to_string(kind)))}};
    }
    return {
        {"extensionKinds", kinds},
        {"ignoredNames", toStringList(s.ignored_names)},
        {"ignoredPrefixes", toStringList(s.ignored_prefixes)},
        {"syncOnStartup", s.sync_on_startup},
        {"settleWindowSeconds", qint64(s.settle_window_seconds)},
        {"massRemovalThresholdPercent", s.mass_removal_threshold_percent},
        {"batchMaxRows", s.batch_max_rows},
        {"batchMaxMilliseconds", s.batch_max_milliseconds},
        {"progressIntervalFiles", s.progress_interval_files},
        {"relocationSampleSize", s.relocation_sample_size},
        {"relocationMinMatchPercent", s.relocation_min_match_percent},
    };
}

// Overlays the keys present in `map` onto `s`. Returns a problem in plain words, or "".
// Range checks are left to hoardor (file::validate inside save_settings).
QString applyMap(file::Settings& s, const QVariantMap& map) {
    if (map.contains("extensionKinds")) {
        std::unordered_map<std::string, file::FileKind> kinds;
        for (const QVariant& row : map.value("extensionKinds").toList()) {
            const QVariantMap r = row.toMap();
            const QString extension = r.value("extension").toString().trimmed();
            if (extension.isEmpty()) continue;
            auto kind = file::file_kind_from_string(r.value("kind").toString().toStdString());
            if (!kind) return QStringLiteral("The extension “%1” has an unknown kind.").arg(extension);
            kinds[extension.toStdString()] = *kind;
        }
        s.extension_kinds = std::move(kinds);
    }
    if (map.contains("ignoredNames")) s.ignored_names = fromStringList(map.value("ignoredNames").toStringList());
    if (map.contains("ignoredPrefixes")) s.ignored_prefixes = fromStringList(map.value("ignoredPrefixes").toStringList());
    if (map.contains("syncOnStartup")) s.sync_on_startup = map.value("syncOnStartup").toBool();

    const auto number = [&map](const char* key, auto& field) {
        if (map.contains(key)) field = static_cast<std::remove_reference_t<decltype(field)>>(map.value(key).toLongLong());
    };
    number("settleWindowSeconds", s.settle_window_seconds);
    number("massRemovalThresholdPercent", s.mass_removal_threshold_percent);
    number("batchMaxRows", s.batch_max_rows);
    number("batchMaxMilliseconds", s.batch_max_milliseconds);
    number("progressIntervalFiles", s.progress_interval_files);
    number("relocationSampleSize", s.relocation_sample_size);
    number("relocationMinMatchPercent", s.relocation_min_match_percent);
    return {};
}

QVariantMap rootToMap(const file::Root& root) {
    return {
        {"id", qint64(root.id)},
        {"categoryId", qint64(root.category_id)},
        {"name", qstr(root.name)},
        {"path", qstr(root.path)},
        {"status", statusName(root.status)},
        {"useMarker", root.use_marker},
        {"caseSensitive", root.case_sensitive},
        {"fileCount", qint64(root.file_count)},
        {"lastSync", nsToMs(root.last_sync_ns)},
        {"heldRemovals", qint64(root.held_removals)},
    };
}

}  // namespace

LibraryBridge::LibraryBridge(const QString& dataDir, QObject* parent) : QObject(parent) {
    // While a sync runs, poll SyncWorker::idle(): its "finished" callback runs before
    // the worker clears its running flag, so the callback alone can't say "all done".
    idleTimer_.setInterval(100);
    connect(&idleTimer_, &QTimer::timeout, this, &LibraryBridge::checkIdle);
    open(dataDir);
}

LibraryBridge::~LibraryBridge() {
    // Stops and joins hoardor's thread first, so no callback runs while the rest goes away.
    worker_.reset();
    if (instance_ == this) instance_ = nullptr;
}

QString LibraryBridge::defaultDataDir() {
    const QString overridden = qEnvironmentVariable("TYLI_DATA_DIR");
    if (!overridden.isEmpty()) return overridden;
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
}

LibraryBridge* LibraryBridge::create(QQmlEngine*, QJSEngine* jsEngine) {
    Q_ASSERT_X(instance_, "LibraryBridge::create", "construct the bridge and call setInstance() before loading QML");
    Q_ASSERT(jsEngine->thread() == instance_->thread());
    // main() owns the bridge; without this the QML engine would delete it.
    QJSEngine::setObjectOwnership(instance_, QJSEngine::CppOwnership);
    return instance_;
}

void LibraryBridge::open(const QString& dataDir) {
    if (!QDir().mkpath(dataDir)) {
        openError_ = tr("The data folder can't be created.");
        databasePath_ = dataDir;
        return;
    }
    databasePath_ = QDir(dataDir).filePath(QStringLiteral("library.db"));
    const std::filesystem::path file = databasePath_.toStdU16String();

    auto database = hoardor::db::Database::open(file);
    if (!database) {
        openError_ = qstr(database.error().message);
        return;
    }
    database_.emplace(std::move(*database));
    auto library = file::Library::open(*database_);
    if (!library) {
        openError_ = qstr(library.error().message);
        return;
    }

    // The callbacks run on hoardor's worker thread. Each copies what it needs and hands it
    // to the UI thread with a queued call, where everything else happens.
    hoardor::master::SyncCallbacks callbacks;
    callbacks.on_progress = [this](const file::SyncProgress& progress) {
        QMetaObject::invokeMethod(this, [this, progress] { onProgress(progress); }, Qt::QueuedConnection);
    };
    callbacks.on_finished = [this](std::optional<file::CategoryId> scope, const file::SyncReport& report) {
        QMetaObject::invokeMethod(this, [this, scope, report] { onFinished(scope, report); }, Qt::QueuedConnection);
    };
    auto worker = hoardor::master::SyncWorker::start(file, std::move(callbacks));
    if (!worker) {
        openError_ = qstr(worker.error().message);
        return;
    }
    library_.emplace(std::move(*library));
    worker_ = std::move(*worker);

    reloadCategories();
    reloadFolders();
    reloadSettings();
    if (!worker_->idle()) {  // "sync at startup" queued a global sync
        setSyncing(true);
        idleTimer_.start();
    }
}

template <class T>
bool LibraryBridge::check(const file::Result<T>& result) {
    if (result) return true;
    emit errorOccurred(qstr(result.error().message));
    return false;
}

void LibraryBridge::reloadCategories() {
    auto all = library_->categories();
    if (!check(all)) return;
    QVariantList out;
    for (const auto& category : *all) {
        out << QVariantMap{{"id", qint64(category.id)}, {"name", qstr(category.name)}, {"kinds", kindNames(category.kinds)}};
    }
    categories_ = out;
    emit categoriesChanged();
}

void LibraryBridge::reloadFolders() {
    auto all = library_->roots();
    if (!check(all)) return;
    QVariantList out;
    for (const auto& root : *all) out << rootToMap(root);
    folders_ = out;
    emit foldersChanged();
}

void LibraryBridge::reloadSettings() {
    auto loaded = library_->load_settings();
    if (!check(loaded)) return;
    settings_ = settingsToMap(*loaded);
    emit settingsChanged();
}

QVariantMap LibraryBridge::settingLimits() const {
    const auto range = [](file::SettingRange r) { return QVariantMap{{"min", qint64(r.min)}, {"max", qint64(r.max)}}; };
    namespace l = file::limits;
    return {
        {"settleWindowSeconds", range(l::settle_window_seconds)},
        {"massRemovalThresholdPercent", range(l::mass_removal_threshold_percent)},
        {"batchMaxRows", range(l::batch_max_rows)},
        {"batchMaxMilliseconds", range(l::batch_max_milliseconds)},
        {"progressIntervalFiles", range(l::progress_interval_files)},
        {"relocationSampleSize", range(l::relocation_sample_size)},
        {"relocationMinMatchPercent", range(l::relocation_min_match_percent)},
    };
}

QStringList LibraryBridge::fileKinds() const {
    using K = file::FileKind;
    return kindNames({K::Audio, K::Video, K::Text, K::Image, K::Subtitle});
}

QVariantMap LibraryBridge::categoryReports() const {
    // Each folder's latest report, grouped by the folder's current category.
    QVariantMap out;
    for (const QVariant& f : folders_) {
        const QVariantMap folder = f.toMap();
        const auto it = folderReports_.constFind(folder.value("id").toLongLong());
        if (it == folderReports_.cend()) continue;
        const QString key = folder.value("categoryId").toString();
        QVariantMap category = out.value(key).toMap();
        QVariantList reports = category.value("folders").toList();
        reports << *it;
        category["folders"] = reports;
        for (const char* counter : {"added", "modified", "removed", "errors"}) {
            category[counter] = category.value(counter).toLongLong() + it->value(counter).toLongLong();
        }
        category["finishedAt"] = std::max(category.value("finishedAt").toLongLong(), it->value("finishedAt").toLongLong());
        category["cancelled"] = category.value("cancelled").toBool() || it->value("outcome") == "cancelled";
        out[key] = category;
    }
    return out;
}

QVariantMap LibraryBridge::folderReports() const {
    QVariantMap out;
    for (auto it = folderReports_.cbegin(); it != folderReports_.cend(); ++it) out[QString::number(it.key())] = *it;
    return out;
}

QString LibraryBridge::folderName(qint64 folderId) const {
    for (const QVariant& f : folders_) {
        const QVariantMap folder = f.toMap();
        if (folder.value("id").toLongLong() == folderId) return folder.value("name").toString();
    }
    return {};
}

// ---------------------------------------------------------------- Sync

bool LibraryBridge::requestSync(int categoryId) {
    if (!worker_) return false;
    const auto scope = categoryId < 0 ? std::nullopt : std::optional<file::CategoryId>(categoryId);
    if (!worker_->request_sync(scope)) return false;
    setSyncing(true);
    idleTimer_.start();
    return true;
}

void LibraryBridge::cancelSync() {
    if (worker_) worker_->cancel();
}

void LibraryBridge::setSyncing(bool syncing) {
    if (syncing_ == syncing) return;
    syncing_ = syncing;
    emit syncingChanged();
}

void LibraryBridge::onProgress(const file::SyncProgress& progress) {
    if (!syncing_) return;  // a late event from a sync that already finished
    syncProgress_ = {
        {"folderIndex", qint64(progress.root_index + 1)},
        {"folderCount", qint64(progress.root_count)},
        {"folderId", qint64(progress.root_id)},
        {"folderName", folderName(progress.root_id)},
        {"files", qint64(progress.scan.files_emitted)},
        {"errors", qint64(progress.scan.errors)},
    };
    emit syncProgressChanged();
}

void LibraryBridge::onFinished(std::optional<file::CategoryId> scope, const file::SyncReport& report) {
    const qint64 finishedAt = QDateTime::currentMSecsSinceEpoch();
    QVariantList reports;
    for (const auto& r : report.roots) {
        const QVariantMap map{
            {"folderId", qint64(r.root_id)},
            {"folderName", folderName(r.root_id)},
            {"outcome", outcomeName(r.outcome)},
            {"added", qint64(r.added)},
            {"modified", qint64(r.modified)},
            {"removed", qint64(r.removed)},
            {"unchanged", qint64(r.unchanged)},
            {"unsettled", qint64(r.unsettled)},
            {"errors", qint64(r.errors)},
            {"relocated", r.relocated},
            {"removalsHeld", r.removals_held},
            {"heldRemovals", qint64(r.held_removals)},
            {"message", qstr(r.message)},
            {"finishedAt", finishedAt},
        };
        folderReports_[r.root_id] = map;
        reports << map;
    }
    lastReport_ = {
        {"categoryId", scope ? qint64(*scope) : qint64(-1)},
        {"cancelled", report.cancelled},
        {"finishedAt", finishedAt},
        {"folders", reports},
    };
    reloadFolders();  // status, path (relocated), counts, last sync time
    emit lastReportChanged();
    emit syncFinished(lastReport_);
}

void LibraryBridge::checkIdle() {
    if (worker_ && !worker_->idle()) return;
    idleTimer_.stop();
    syncProgress_.clear();
    emit syncProgressChanged();
    setSyncing(false);
}

// ---------------------------------------------------------------- Folders

void LibraryBridge::runFolderOperation(std::function<QString(file::Library&, QString&)> work) {
    ++folderOperations_;
    emit folderOperationRunningChanged();

    // Its own connection: a Database must only be used from one thread at a time.
    const QString path = databasePath_;
    auto future = QtConcurrent::run([path, work = std::move(work)]() -> std::pair<bool, QString> {
        auto database = hoardor::db::Database::open(path.toStdU16String());
        if (!database) return {false, qstr(database.error().message)};
        auto library = file::Library::open(*database);
        if (!library) return {false, qstr(library.error().message)};
        QString error;
        QString message = work(*library, error);
        if (!error.isEmpty()) return {false, error};
        return {true, message};
    });
    // The watcher is a child of the bridge: if the bridge goes away first, no signal arrives.
    auto* watcher = new QFutureWatcher<std::pair<bool, QString>>(this);
    connect(watcher, &QFutureWatcherBase::finished, this, [this, watcher] {
        const auto [ok, message] = watcher->result();
        watcher->deleteLater();
        --folderOperations_;
        emit folderOperationRunningChanged();
        if (ok) reloadFolders();
        emit folderOperationFinished(ok, message);
    });
    watcher->setFuture(future);
}

void LibraryBridge::addFolder(int categoryId, const QString& folder, const QString& name, bool useMarker) {
    if (!library_) return;
    const std::filesystem::path path = toPath(folder);
    const std::string label = name.trimmed().toStdString();
    runFolderOperation([categoryId, path, label, useMarker](file::Library& library, QString& error) {
        auto root = library.add_root(categoryId, path, label, useMarker);
        if (!root) {
            error = qstr(root.error().message);
            return QString();
        }
        QString message = tr("Added “%1”.").arg(qstr(root->name));
        if (useMarker && !root->use_marker) {
            message += QLatin1Char(' ') + tr("No .hoardor-root marker could be written (the folder is read-only), so "
                                             "it's recognized by its location instead.");
        }
        return message;
    });
}

void LibraryBridge::relocateFolder(int folderId, const QString& folder) {
    if (!library_) return;
    const std::filesystem::path path = toPath(folder);
    runFolderOperation([folderId, path](file::Library& library, QString& error) {
        auto root = library.relocate_root(folderId, path);
        if (!root) {
            error = qstr(root.error().message);
            return QString();
        }
        return tr("“%1” now points to %2.").arg(qstr(root->name), qstr(root->path));
    });
}

bool LibraryBridge::removeFolder(int folderId) {
    if (!library_ || !check(library_->remove_root(folderId))) return false;
    folderReports_.remove(folderId);
    reloadFolders();
    emit lastReportChanged();
    return true;
}

bool LibraryBridge::setFolderCategory(int folderId, int categoryId) {
    if (!library_ || !check(library_->set_root_category(folderId, categoryId))) return false;
    reloadFolders();
    emit lastReportChanged();  // the folder's report moves to its new category
    return true;
}

bool LibraryBridge::setFolderCaseSensitive(int folderId, bool caseSensitive) {
    if (!library_ || !check(library_->set_root_case_sensitive(folderId, caseSensitive))) return false;
    reloadFolders();
    return true;
}

bool LibraryBridge::applyHeldRemovals(int folderId) {
    if (!library_) return false;
    auto removed = library_->apply_held_removals(folderId);
    if (!check(removed)) return false;
    if (auto it = folderReports_.find(folderId); it != folderReports_.end()) {
        (*it)["removalsHeld"] = false;
        (*it)["heldRemovals"] = 0;
        (*it)["removed"] = it->value("removed").toLongLong() + qint64(*removed);
    }
    reloadFolders();
    emit lastReportChanged();
    return true;
}

QVariantList LibraryBridge::scanErrors(int folderId) {
    if (!library_) return {};
    auto errors = library_->scan_errors(folderId);
    if (!errors) return {};
    QVariantList out;
    for (const auto& e : *errors) {
        out << QVariantMap{{"path", qstr(e.relative_path)}, {"isDirectory", e.is_directory}, {"message", qstr(e.message)}};
    }
    return out;
}

QString LibraryBridge::localPath(const QUrl& url) const { return QDir::toNativeSeparators(url.toLocalFile()); }

// ---------------------------------------------------------------- Categories

bool LibraryBridge::addCategory(const QString& name, const QStringList& kinds) {
    if (!library_) return false;
    auto parsed = parseKinds(kinds);
    if (!parsed) {
        emit errorOccurred(tr("Unknown file kind."));
        return false;
    }
    if (!check(library_->add_category(name.toStdString(), *parsed))) return false;
    reloadCategories();
    return true;
}

bool LibraryBridge::updateCategory(int id, const QString& name, const QStringList& kinds) {
    if (!library_) return false;
    auto parsed = parseKinds(kinds);
    if (!parsed) {
        emit errorOccurred(tr("Unknown file kind."));
        return false;
    }
    if (!check(library_->update_category(file::Category{id, name.toStdString(), *parsed}))) return false;
    reloadCategories();
    return true;
}

bool LibraryBridge::removeCategory(int id) {
    if (!library_ || !check(library_->remove_category(id))) return false;
    reloadCategories();
    return true;
}

// ---------------------------------------------------------------- Settings

bool LibraryBridge::saveSettings(const QVariantMap& settings) {
    if (!library_) return false;
    auto current = library_->load_settings();
    if (!check(current)) return false;
    if (const QString problem = applyMap(*current, settings); !problem.isEmpty()) {
        emit errorOccurred(problem);
        return false;
    }
    if (!check(library_->save_settings(*current))) return false;
    reloadSettings();
    return true;
}

QVariantMap LibraryBridge::defaultSettings() const { return settingsToMap(file::Settings::defaults()); }

}
