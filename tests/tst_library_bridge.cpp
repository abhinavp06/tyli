#include "library_bridge.hpp"
#include "test_support.hpp"

#include <QSignalSpy>
#include <QElapsedTimer>
#include <QTemporaryDir>
#include <QTest>

#include <memory>

using namespace tyli;
using namespace tyli::test;

class TestLibraryBridge : public QObject {
    Q_OBJECT

private:
    std::unique_ptr<QTemporaryDir> data_;
    std::unique_ptr<QTemporaryDir> media_;
    std::unique_ptr<LibraryBridge> bridge_;
    int music_ = -1;

    QString media(const QString& relative) const { return media_->filePath(relative); }

private slots:
    void init() {
        data_ = std::make_unique<QTemporaryDir>();
        media_ = std::make_unique<QTemporaryDir>();
        bridge_ = std::make_unique<LibraryBridge>(data_->path());
        QVERIFY2(bridge_->ready(), qPrintable(bridge_->openError()));
        music_ = categoryId(*bridge_, "Music");
        // Freshly written files would otherwise count as "still being copied".
        QVERIFY(bridge_->saveSettings({{"settleWindowSeconds", 0}}));
    }

    void cleanup() {
        bridge_.reset();
        media_.reset();
        data_.reset();
    }

    void exposesTheDefaultCategories() {
        QVERIFY(music_ > 0);
        QVERIFY(categoryId(*bridge_, "Movies") > 0);
        const QVariantMap music = bridge_->categories().first().toMap();
        QCOMPARE(music.value("kinds").toStringList(), QStringList({"audio", "image"}));
        QVERIFY(bridge_->databasePath().endsWith("library.db"));
    }

    void addFolderRunsInTheBackgroundAndListsIt() {
        writeTree(media("Music"), 3);
        QSignalSpy running(bridge_.get(), &LibraryBridge::folderOperationRunningChanged);
        const auto [ok, message] = addFolder(*bridge_, music_, media("Music"), "My music");
        QVERIFY2(ok, qPrintable(message));
        QVERIFY(message.contains("My music"));
        QCOMPARE(running.count(), 2);  // on, then off
        QVERIFY(!bridge_->folderOperationRunning());

        const QVariantMap f = folder(*bridge_, "My music");
        QCOMPARE(f.value("categoryId").toInt(), music_);
        QCOMPARE(f.value("path").toString(), media("Music"));
        QVERIFY(f.value("useMarker").toBool());
        QVERIFY(QFile::exists(media("Music/.hoardor-root")));
    }

    void addFolderAcceptsFileUrls() {
        writeTree(media("Music"), 1);
        const auto [ok, message] = addFolder(*bridge_, music_, QUrl::fromLocalFile(media("Music")).toString(), "Url");
        QVERIFY2(ok, qPrintable(message));
        QCOMPARE(folder(*bridge_, "Url").value("path").toString(), media("Music"));
    }

    void addFolderWithoutMarker() {
        writeTree(media("Music"), 1);
        QVERIFY(addFolder(*bridge_, music_, media("Music"), "No marker", false).first);
        QVERIFY(!folder(*bridge_, "No marker").value("useMarker").toBool());
        QVERIFY(!QFile::exists(media("Music/.hoardor-root")));
    }

    void addFolderReportsHoardorsErrors() {
        writeTree(media("Music/Inner"), 1);
        // Missing folder
        auto [ok, message] = addFolder(*bridge_, music_, media("does not exist"));
        QVERIFY(!ok);
        QVERIFY(!message.isEmpty());
        // Already a folder, and overlapping folders
        QVERIFY(addFolder(*bridge_, music_, media("Music")).first);
        QVERIFY(!addFolder(*bridge_, music_, media("Music")).first);
        std::tie(ok, message) = addFolder(*bridge_, music_, media("Music/Inner"));
        QVERIFY(!ok);
        QVERIFY(!message.isEmpty());
        QCOMPARE(bridge_->folders().size(), 1);
    }

    void unicodePathsRoundTrip() {
        const QString dir = media(QString::fromUtf8("Música ✓/Ünïcødé"));
        writeFile(dir + QString::fromUtf8("/Café.flac"));
        QVERIFY(addFolder(*bridge_, music_, dir, QString::fromUtf8("Música")).first);
        QCOMPARE(folder(*bridge_, QString::fromUtf8("Música")).value("path").toString(), dir);
        const QVariantMap report = sync(*bridge_, music_);
        QCOMPARE(folderReport(report).value("added").toInt(), 1);
    }

    void syncReportsWhatChanged() {
        writeTree(media("Music"), 5);
        QVERIFY(addFolder(*bridge_, music_, media("Music"), "M").first);
        QSignalSpy syncing(bridge_.get(), &LibraryBridge::syncingChanged);

        QVariantMap report = sync(*bridge_, music_);
        QCOMPARE(syncing.count(), 2);  // true, then false
        QCOMPARE(report.value("categoryId").toInt(), music_);
        QCOMPARE(report.value("cancelled").toBool(), false);
        QVariantMap r = folderReport(report);
        QCOMPARE(r.value("outcome").toString(), "synced");
        QCOMPARE(r.value("added").toInt(), 5);
        QCOMPARE(r.value("folderName").toString(), "M");

        QVariantMap f = folder(*bridge_, "M");
        QCOMPARE(f.value("status").toString(), "online");
        QCOMPARE(f.value("fileCount").toInt(), 5);
        QVERIFY(f.value("lastSync").toLongLong() > 0);

        // One added, one modified (new size), one removed.
        writeFile(media("Music/album 0/new.flac"));
        writeFile(media("Music/album 0/track 00000.flac"), 99);
        QVERIFY(QFile::remove(media("Music/album 0/track 00001.flac")));
        report = sync(*bridge_, music_);
        r = folderReport(report);
        QCOMPARE(r.value("added").toInt(), 1);
        QCOMPARE(r.value("modified").toInt(), 1);
        QCOMPARE(r.value("removed").toInt(), 1);
        QCOMPARE(r.value("unchanged").toInt(), 3);

        // The section's summary comes from categoryReports.
        const QVariantMap category = bridge_->categoryReports().value(QString::number(music_)).toMap();
        QCOMPARE(category.value("added").toInt(), 1);
        QCOMPARE(category.value("folders").toList().size(), 1);
        QVERIFY(bridge_->syncProgress().isEmpty());
    }

    void syncOfEverything() {
        writeTree(media("Music"), 2);
        writeFile(media("Movies/film.mkv"));
        QVERIFY(addFolder(*bridge_, music_, media("Music"), "M").first);
        QVERIFY(addFolder(*bridge_, categoryId(*bridge_, "Movies"), media("Movies"), "F").first);
        const QVariantMap report = sync(*bridge_, -1);
        QCOMPARE(report.value("categoryId").toInt(), -1);
        QCOMPARE(report.value("folders").toList().size(), 2);
        QCOMPARE(bridge_->categoryReports().size(), 2);
    }

    void aSecondRequestForTheSameScopeIsIgnored() {
        writeTree(media("Music"), 2000);
        QVERIFY(addFolder(*bridge_, music_, media("Music")).first);
        QVERIFY(bridge_->requestSync(music_));
        QVERIFY(!bridge_->requestSync(music_));
        QTRY_VERIFY_WITH_TIMEOUT(!bridge_->syncing(), 20000);
    }

    void cancelRemovesNothing() {
        writeTree(media("Music"), 20000);
        QVERIFY(bridge_->saveSettings({{"progressIntervalFiles", 1}}));
        QVERIFY(addFolder(*bridge_, music_, media("Music"), "Big").first);

        QSignalSpy progress(bridge_.get(), &LibraryBridge::syncProgressChanged);
        QSignalSpy finished(bridge_.get(), &LibraryBridge::syncFinished);
        QVERIFY(bridge_->requestSync(music_));
        QVERIFY(progress.wait(10000));
        QVERIFY(!bridge_->syncProgress().isEmpty());
        QCOMPARE(bridge_->syncProgress().value("folderName").toString(), "Big");
        bridge_->cancelSync();

        QVERIFY(finished.wait(20000) || finished.count() == 1);
        QTRY_VERIFY_WITH_TIMEOUT(!bridge_->syncing(), 5000);
        const QVariantMap report = finished.first().at(0).toMap();
        QVERIFY(report.value("cancelled").toBool());
        QCOMPARE(folderReport(report).value("outcome").toString(), "cancelled");
        QCOMPARE(folderReport(report).value("removed").toInt(), 0);
        QVERIFY(bridge_->categoryReports().value(QString::number(music_)).toMap().value("cancelled").toBool());
    }

    void anUnpluggedFolderIsOfflineAndKeepsItsFiles() {
        writeTree(media("Drive/Music"), 4);
        QVERIFY(addFolder(*bridge_, music_, media("Drive/Music"), "Ext").first);
        QVERIFY(!sync(*bridge_, music_).isEmpty());
        QVERIFY(QDir().rename(media("Drive"), media("Unplugged")));

        const QVariantMap r = folderReport(sync(*bridge_, music_));
        QCOMPARE(r.value("outcome").toString(), "offline");
        QVERIFY(!r.value("message").toString().isEmpty());
        QCOMPARE(r.value("removed").toInt(), 0);
        const QVariantMap f = folder(*bridge_, "Ext");
        QCOMPARE(f.value("status").toString(), "offline");
        QCOMPARE(f.value("fileCount").toInt(), 4);
    }

    void relocateAFolder() {
        writeTree(media("Old/Music"), 3);
        QVERIFY(addFolder(*bridge_, music_, media("Old/Music"), "Moved").first);
        QVERIFY(!sync(*bridge_, music_).isEmpty());
        QVERIFY(QDir().rename(media("Old"), media("New")));

        QSignalSpy done(bridge_.get(), &LibraryBridge::folderOperationFinished);
        bridge_->relocateFolder(folder(*bridge_, "Moved").value("id").toInt(), media("New/Music"));
        QVERIFY(done.wait(10000));
        QVERIFY2(done.first().at(0).toBool(), qPrintable(done.first().at(1).toString()));
        QCOMPARE(folder(*bridge_, "Moved").value("path").toString(), media("New/Music"));

        // Relocating to a different folder is refused (the marker doesn't match).
        writeTree(media("Other"), 1);
        QVERIFY(addFolder(*bridge_, music_, media("Other"), "Other").first);
        done.clear();
        bridge_->relocateFolder(folder(*bridge_, "Moved").value("id").toInt(), media("Other"));
        QVERIFY(done.wait(10000));
        QVERIFY(!done.first().at(0).toBool());
    }

    void heldRemovalsWaitForConfirmation() {
        writeTree(media("Music"), 8);
        QVERIFY(addFolder(*bridge_, music_, media("Music"), "Held").first);
        QVERIFY(!sync(*bridge_, music_).isEmpty());
        for (int i = 0; i < 4; ++i) {
            QVERIFY(QFile::remove(media(QStringLiteral("Music/album 0/track %1.flac").arg(i, 5, 10, QLatin1Char('0')))));
        }

        const QVariantMap r = folderReport(sync(*bridge_, music_));
        QVERIFY(r.value("removalsHeld").toBool());
        QCOMPARE(r.value("heldRemovals").toInt(), 4);
        QCOMPARE(folder(*bridge_, "Held").value("heldRemovals").toInt(), 4);
        QCOMPARE(folder(*bridge_, "Held").value("fileCount").toInt(), 8);

        QVERIFY(bridge_->applyHeldRemovals(folder(*bridge_, "Held").value("id").toInt()));
        QCOMPARE(folder(*bridge_, "Held").value("heldRemovals").toInt(), 0);
        QCOMPARE(folder(*bridge_, "Held").value("fileCount").toInt(), 4);
        const QVariantMap category = bridge_->categoryReports().value(QString::number(music_)).toMap();
        QCOMPARE(folderReport(category).value("removalsHeld").toBool(), false);
    }

    void scanErrorsListUnreadableFolders() {
        writeTree(media("Music"), 2);
        writeFile(media("Music/Locked/song.flac"));
        QFile::setPermissions(media("Music/Locked"), QFileDevice::Permissions());
        if (!QDir(media("Music/Locked")).entryList(QDir::Files).isEmpty()) {
            QFile::setPermissions(media("Music/Locked"), QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner);
            QSKIP("Permissions don't apply (running as root?)");
        }
        QVERIFY(addFolder(*bridge_, music_, media("Music"), "E").first);
        const QVariantMap r = folderReport(sync(*bridge_, music_));
        QFile::setPermissions(media("Music/Locked"), QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner);
        QCOMPARE(r.value("errors").toInt(), 1);
        const QVariantList errors = bridge_->scanErrors(folder(*bridge_, "E").value("id").toInt());
        QCOMPARE(errors.size(), 1);
        QCOMPARE(errors.first().toMap().value("path").toString(), "Locked");
        QVERIFY(errors.first().toMap().value("isDirectory").toBool());
    }

    void folderChanges() {
        writeTree(media("Music"), 1);
        QVERIFY(addFolder(*bridge_, music_, media("Music"), "F").first);
        const int id = folder(*bridge_, "F").value("id").toInt();
        const int movies = categoryId(*bridge_, "Movies");

        QSignalSpy changed(bridge_.get(), &LibraryBridge::foldersChanged);
        QVERIFY(bridge_->setFolderCategory(id, movies));
        QCOMPARE(folder(*bridge_, "F").value("categoryId").toInt(), movies);
        QVERIFY(bridge_->setFolderCaseSensitive(id, false));
        QVERIFY(!folder(*bridge_, "F").value("caseSensitive").toBool());
        QCOMPARE(changed.count(), 2);

        QSignalSpy errors(bridge_.get(), &LibraryBridge::errorOccurred);
        QVERIFY(!bridge_->setFolderCategory(id, 9999));
        QCOMPARE(errors.count(), 1);

        QVERIFY(bridge_->removeFolder(id));
        QVERIFY(bridge_->folders().isEmpty());
        QVERIFY(QFile::exists(media("Music/album 0/track 00000.flac")));  // files on disk untouched
    }

    void categoriesCrudAndErrors() {
        QSignalSpy errors(bridge_.get(), &LibraryBridge::errorOccurred);
        QSignalSpy changed(bridge_.get(), &LibraryBridge::categoriesChanged);

        QVERIFY(bridge_->addCategory("Podcasts", {"audio"}));
        const int podcasts = categoryId(*bridge_, "Podcasts");
        QVERIFY(podcasts > 0);
        QVERIFY(!bridge_->addCategory("Podcasts", {"audio"}));  // duplicate
        QVERIFY(!bridge_->addCategory("Weird", {"smell"}));     // unknown kind
        QCOMPARE(errors.count(), 2);

        QVERIFY(bridge_->updateCategory(podcasts, "Talks", {"video", "subtitle"}));
        QCOMPARE(categoryId(*bridge_, "Talks"), podcasts);

        writeTree(media("Talks"), 1);
        QVERIFY(addFolder(*bridge_, podcasts, media("Talks")).first);
        QVERIFY(!bridge_->removeCategory(podcasts));  // still has a folder
        QCOMPARE(errors.count(), 3);
        QVERIFY(!errors.last().at(0).toString().isEmpty());

        QVERIFY(bridge_->removeFolder(folder(*bridge_, "Talks").value("id").toInt()));
        QVERIFY(bridge_->removeCategory(podcasts));
        QCOMPARE(categoryId(*bridge_, "Talks"), -1);
        QCOMPARE(changed.count(), 3);  // add, update, remove
    }

    void settingsRoundTripAndValidation() {
        const QVariantMap defaults = bridge_->defaultSettings();
        QCOMPARE(defaults.value("massRemovalThresholdPercent").toInt(), 25);
        QVERIFY(!defaults.value("extensionKinds").toList().isEmpty());
        QCOMPARE(bridge_->settingLimits().value("massRemovalThresholdPercent").toMap().value("max").toInt(), 100);

        QVERIFY(bridge_->saveSettings({{"massRemovalThresholdPercent", 40},
                                       {"syncOnStartup", false},
                                       {"ignoredNames", QStringList{"Thumbs.db", " ", "@eaDir"}},
                                       {"extensionKinds", QVariantList{QVariantMap{{"extension", "flac"}, {"kind", "audio"}},
                                                                       QVariantMap{{"extension", ".MKV"}, {"kind", "video"}}}}}));
        QCOMPARE(bridge_->settings().value("massRemovalThresholdPercent").toInt(), 40);
        QCOMPARE(bridge_->settings().value("ignoredNames").toStringList(), QStringList({"Thumbs.db", "@eaDir"}));
        QCOMPARE(bridge_->settings().value("extensionKinds").toList().size(), 2);

        // Invalid values are refused with hoardor's message, and nothing changes.
        QSignalSpy errors(bridge_.get(), &LibraryBridge::errorOccurred);
        QVERIFY(!bridge_->saveSettings({{"massRemovalThresholdPercent", 150}}));
        QVERIFY(!bridge_->saveSettings({{"extensionKinds", QVariantList{QVariantMap{{"extension", "x"}, {"kind", "nope"}}}}}));
        QCOMPARE(errors.count(), 2);
        QVERIFY(errors.first().at(0).toString().contains("100"));
        QCOMPARE(bridge_->settings().value("massRemovalThresholdPercent").toInt(), 40);

        // Stored: a new bridge on the same data folder reads them back.
        bridge_.reset();
        LibraryBridge reopened(data_->path());
        QCOMPARE(reopened.settings().value("massRemovalThresholdPercent").toInt(), 40);
    }

    void syncAtStartupRunsInTheBackground() {
        writeTree(media("Music"), 3);
        QVERIFY(addFolder(*bridge_, music_, media("Music"), "S").first);
        QVERIFY(bridge_->saveSettings({{"syncOnStartup", true}}));
        bridge_.reset();

        LibraryBridge reopened(data_->path());
        QSignalSpy finished(&reopened, &LibraryBridge::syncFinished);
        QVERIFY(finished.wait(10000) || finished.count() == 1);
        QTRY_VERIFY_WITH_TIMEOUT(!reopened.syncing(), 5000);
        QCOMPARE(finished.first().at(0).toMap().value("categoryId").toInt(), -1);
        QCOMPARE(folder(reopened, "S").value("fileCount").toInt(), 3);
    }

    void anUnopenableDatabaseIsReportedNotCrashed() {
        // The data folder is a file, so it can't be created.
        const QString blocker = media("not-a-folder");
        writeFile(blocker);
        LibraryBridge broken(blocker + "/inside");
        QVERIFY(!broken.ready());
        QVERIFY(!broken.openError().isEmpty());
        QVERIFY(broken.categories().isEmpty());
        QVERIFY(!broken.requestSync(-1));
        QVERIFY(!broken.addCategory("X", {}));
    }

    void destroyingMidSyncStopsPromptly() {
        writeTree(media("Music"), 20000);
        QVERIFY(addFolder(*bridge_, music_, media("Music")).first);
        QVERIFY(bridge_->requestSync(music_));
        QElapsedTimer timer;
        timer.start();
        bridge_.reset();  // cancels and joins hoardor's thread
        QVERIFY(timer.elapsed() < 2000);
    }
};

QTEST_MAIN(TestLibraryBridge)
#include "tst_library_bridge.moc"
