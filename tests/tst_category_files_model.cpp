#include "category_files_model.hpp"
#include "library_bridge.hpp"
#include "test_support.hpp"

#include <QSet>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

#include <memory>

using namespace tyli;
using namespace tyli::test;

class TestCategoryFilesModel : public QObject {
    Q_OBJECT

private:
    std::unique_ptr<QTemporaryDir> data_;
    std::unique_ptr<QTemporaryDir> media_;
    std::unique_ptr<LibraryBridge> bridge_;
    int music_ = -1;

    QString media(const QString& relative) const { return media_->filePath(relative); }

    bool addAndSync(const QString& relative, const QString& name, int category = -1) {
        if (!addFolder(*bridge_, category < 0 ? music_ : category, media(relative), name).first) return false;
        return !sync(*bridge_, category < 0 ? music_ : category).isEmpty();
    }

    static void fetchAll(CategoryFilesModel& model) {
        while (model.canFetchMore({})) model.fetchMore({});
    }

    static QStringList column(const CategoryFilesModel& model, int role) {
        QStringList out;
        for (int row = 0; row < model.rowCount(); ++row) out << model.data(model.index(row), role).toString();
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

    void pagesAcrossFoldersWithoutGapsOrDuplicates() {
        writeTree(media("A"), 250);
        writeTree(media("B"), 300);
        QVERIFY(addAndSync("A", "A"));
        QVERIFY(addAndSync("B", "B"));

        CategoryFilesModel model;
        QVERIFY(!model.canFetchMore({}));  // no category yet
        model.setCategoryId(music_);
        QVERIFY(model.canFetchMore({}));

        QSignalSpy inserted(&model, &QAbstractItemModel::rowsInserted);
        model.fetchMore({});
        QCOMPARE(model.rowCount(), CategoryFilesModel::PageSize);
        model.fetchMore({});  // crosses from folder A into folder B
        QCOMPARE(model.rowCount(), 2 * CategoryFilesModel::PageSize);
        fetchAll(model);
        QCOMPARE(model.rowCount(), 550);
        QCOMPARE(inserted.count(), 3);  // 200 + 200 + 150
        QVERIFY(!model.canFetchMore({}));

        // Folder by folder: A's 250 files, then B's 300.
        const QStringList folders = column(model, CategoryFilesModel::FolderNameRole);
        QCOMPARE(folders.count("A"), 250);
        QCOMPARE(folders.count("B"), 300);
        QCOMPARE(folders.indexOf("B"), 250);
        QSet<QString> unique;
        for (int row = 0; row < model.rowCount(); ++row) {
            unique.insert(folders[row] + "/" + model.data(model.index(row), CategoryFilesModel::RelativePathRole).toString());
        }
        QCOMPARE(unique.size(), 550);
    }

    void rolesCarryTheEntryAndItsFolder() {
        writeFile(media("Music/a/song.flac"), 1234);
        QVERIFY(addAndSync("Music", "Mine"));
        CategoryFilesModel model;
        model.setCategoryId(music_);
        fetchAll(model);
        QCOMPARE(model.rowCount(), 1);
        const QModelIndex index = model.index(0);
        QCOMPARE(model.data(index, CategoryFilesModel::RelativePathRole).toString(), "a/song.flac");
        QCOMPARE(model.data(index, CategoryFilesModel::FolderNameRole).toString(), "Mine");
        QVERIFY(model.data(index, CategoryFilesModel::FolderOnlineRole).toBool());
        QCOMPARE(model.data(index, CategoryFilesModel::SizeRole).toLongLong(), 1234);
        QCOMPARE(model.data(index, CategoryFilesModel::KindRole).toString(), "audio");
        QVERIFY(model.data(index, CategoryFilesModel::UnsettledRole).toBool());  // written just now
        QVERIFY(model.data(index, CategoryFilesModel::MtimeRole).toLongLong() > 0);
        QCOMPARE(model.roleNames().value(CategoryFilesModel::FolderOnlineRole), QByteArray("folderOnline"));
        QVERIFY(!model.data(model.index(3), CategoryFilesModel::RelativePathRole).isValid());
    }

    void anOfflineFolderStillListsItsFilesMarkedOffline() {
        writeTree(media("Drive/Music"), 30);
        QVERIFY(addAndSync("Drive/Music", "Ext"));
        QVERIFY(QDir().rename(media("Drive"), media("Gone")));
        QVERIFY(!sync(*bridge_, music_).isEmpty());

        CategoryFilesModel model;
        model.setCategoryId(music_);
        fetchAll(model);
        QCOMPARE(model.rowCount(), 30);
        QVERIFY(!model.data(model.index(0), CategoryFilesModel::FolderOnlineRole).toBool());
    }

    void reloadsByItselfWhenFoldersChange() {
        writeTree(media("A"), 10);
        QVERIFY(addAndSync("A", "A"));
        CategoryFilesModel model;
        model.setCategoryId(music_);
        fetchAll(model);
        QCOMPARE(model.rowCount(), 10);

        QSignalSpy reset(&model, &QAbstractItemModel::modelReset);
        writeTree(media("B"), 5);
        QVERIFY(addAndSync("B", "B"));  // foldersChanged: added, then synced
        QVERIFY(reset.count() >= 1);
        fetchAll(model);
        QCOMPARE(model.rowCount(), 15);
    }

    void switchingCategories() {
        writeTree(media("Music"), 12);
        writeFile(media("Films/film.mkv"));
        const int movies = categoryId(*bridge_, "Movies");
        QVERIFY(addAndSync("Music", "M"));
        QVERIFY(addAndSync("Films", "F", movies));

        CategoryFilesModel model;
        QSignalSpy changed(&model, &CategoryFilesModel::categoryIdChanged);
        model.setCategoryId(music_);
        fetchAll(model);
        QCOMPARE(model.rowCount(), 12);
        model.setCategoryId(movies);
        QCOMPARE(model.rowCount(), 0);
        fetchAll(model);
        QCOMPARE(model.rowCount(), 1);
        QCOMPARE(model.data(model.index(0), CategoryFilesModel::KindRole).toString(), "video");
        model.setCategoryId(movies);  // same category: nothing happens
        QCOMPARE(changed.count(), 2);
    }

    void emptyAndUnknownCategories() {
        CategoryFilesModel model;
        model.setCategoryId(categoryId(*bridge_, "Books"));  // no folders
        QVERIFY(!model.canFetchMore({}));
        model.setCategoryId(4242);  // no such category
        fetchAll(model);
        QCOMPARE(model.rowCount(), 0);
    }
};

QTEST_MAIN(TestCategoryFilesModel)
#include "tst_category_files_model.moc"
