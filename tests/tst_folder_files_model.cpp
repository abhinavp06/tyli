#include "folder_files_model.hpp"
#include "library_bridge.hpp"
#include "test_support.hpp"

#include <QSet>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

#include <memory>

using namespace tyli;
using namespace tyli::test;

class TestFolderFilesModel : public QObject {
    Q_OBJECT

private:
    std::unique_ptr<QTemporaryDir> data_;
    std::unique_ptr<QTemporaryDir> media_;
    std::unique_ptr<LibraryBridge> bridge_;
    int music_ = -1;

    QString media(const QString& relative) const { return media_->filePath(relative); }

    int addAndSync(const QString& relative, const QString& name) {
        if (!addFolder(*bridge_, music_, media(relative), name).first) return -1;
        if (sync(*bridge_, music_).isEmpty()) return -1;
        return folder(*bridge_, name).value("id").toInt();
    }

    static void fetchAll(FolderFilesModel& model) {
        while (model.canFetchMore({})) model.fetchMore({});
    }

    static QStringList paths(const FolderFilesModel& model) {
        QStringList out;
        for (int row = 0; row < model.rowCount(); ++row) {
            out << model.data(model.index(row), FolderFilesModel::RelativePathRole).toString();
        }
        return out;
    }

private slots:
    void init() {
        data_ = std::make_unique<QTemporaryDir>();
        media_ = std::make_unique<QTemporaryDir>();
        bridge_ = std::make_unique<LibraryBridge>(data_->path());
        LibraryBridge::setInstance(bridge_.get());
        music_ = categoryId(*bridge_, "Music");
    }

    void cleanup() {
        bridge_.reset();
        media_.reset();
        data_.reset();
    }

    void pagesThroughAThousandFilesWithoutGapsOrDuplicates() {
        writeTree(media("Music"), 1000);
        const int id = addAndSync("Music", "M");
        QVERIFY(id > 0);

        FolderFilesModel model;
        QCOMPARE(model.rowCount(), 0);
        QVERIFY(!model.canFetchMore({}));  // no folder yet
        model.setFolderId(id);
        QVERIFY(model.canFetchMore({}));

        QSignalSpy inserted(&model, &QAbstractItemModel::rowsInserted);
        model.fetchMore({});
        QCOMPARE(model.rowCount(), FolderFilesModel::PageSize);
        QCOMPARE(model.count(), FolderFilesModel::PageSize);
        fetchAll(model);
        QCOMPARE(model.rowCount(), 1000);
        QCOMPARE(inserted.count(), 5);  // five pages of 200
        QVERIFY(!model.canFetchMore({}));

        const QStringList all = paths(model);
        QCOMPARE(QSet<QString>(all.begin(), all.end()).size(), 1000);
        QVERIFY(all.contains("album 9/track 00999.flac"));
    }

    void rolesCarryTheEntry() {
        writeFile(media("Music/a/song.flac"), 1234);
        writeFile(media("Music/a/cover.jpg"), 10);
        const int id = addAndSync("Music", "M");
        FolderFilesModel model;
        model.setFolderId(id);
        fetchAll(model);
        QCOMPARE(model.rowCount(), 2);
        const QHash<int, QByteArray> roles = model.roleNames();
        QCOMPARE(roles.value(FolderFilesModel::UnsettledRole), QByteArray("unsettled"));
        for (int row = 0; row < 2; ++row) {
            const QModelIndex index = model.index(row);
            if (model.data(index, FolderFilesModel::RelativePathRole) == "a/song.flac") {
                QCOMPARE(model.data(index, FolderFilesModel::SizeRole).toLongLong(), 1234);
                QCOMPARE(model.data(index, FolderFilesModel::KindRole).toString(), "audio");
                // Written just now: inside the default settle window.
                QVERIFY(model.data(index, FolderFilesModel::UnsettledRole).toBool());
                QVERIFY(model.data(index, FolderFilesModel::MtimeRole).toLongLong() > 0);
            } else {
                QCOMPARE(model.data(index, FolderFilesModel::KindRole).toString(), "image");
            }
        }
        QVERIFY(!model.data(model.index(5), FolderFilesModel::RelativePathRole).isValid());
    }

    void reloadAfterASyncSeesNewFiles() {
        writeTree(media("Music"), 250);
        const int id = addAndSync("Music", "M");
        FolderFilesModel model;
        model.setFolderId(id);
        fetchAll(model);
        QCOMPARE(model.rowCount(), 250);

        writeTree(media("Music/more"), 10);
        QVERIFY(!sync(*bridge_, music_).isEmpty());
        QSignalSpy reset(&model, &QAbstractItemModel::modelReset);
        model.reload();
        QCOMPARE(reset.count(), 1);
        QCOMPARE(model.rowCount(), 0);
        fetchAll(model);
        QCOMPARE(model.rowCount(), 260);
    }

    void switchingFolders() {
        writeTree(media("A"), 300);
        writeTree(media("B"), 5);
        const int a = addAndSync("A", "A");
        const int b = addAndSync("B", "B");
        FolderFilesModel model;
        QSignalSpy changed(&model, &FolderFilesModel::folderIdChanged);
        model.setFolderId(a);
        fetchAll(model);
        QCOMPARE(model.rowCount(), 300);
        model.setFolderId(b);
        QCOMPARE(model.rowCount(), 0);
        fetchAll(model);
        QCOMPARE(model.rowCount(), 5);
        model.setFolderId(b);  // same folder: nothing happens
        QCOMPARE(model.rowCount(), 5);
        QCOMPARE(changed.count(), 2);
    }

    void anOfflineFolderStillListsItsFiles() {
        writeTree(media("Drive/Music"), 30);
        const int id = addAndSync("Drive/Music", "Ext");
        QVERIFY(QDir().rename(media("Drive"), media("Gone")));
        QVERIFY(!sync(*bridge_, music_).isEmpty());
        QCOMPARE(folder(*bridge_, "Ext").value("status").toString(), "offline");

        FolderFilesModel model;
        model.setFolderId(id);
        fetchAll(model);
        QCOMPARE(model.rowCount(), 30);
    }

    void anUnknownFolderIsEmpty() {
        FolderFilesModel model;
        model.setFolderId(4242);
        fetchAll(model);
        QCOMPARE(model.rowCount(), 0);
    }
};

QTEST_MAIN(TestFolderFilesModel)
#include "tst_folder_files_model.moc"
