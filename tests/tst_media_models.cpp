// GroupModel, ItemModel, the bridge's copy lists and default views, and ThumbnailCache,
// over a real library whose metadata is seeded (test_support.hpp).

#include "app_setup.hpp"
#include "group_model.hpp"
#include "item_model.hpp"
#include "library_bridge.hpp"
#include "test_support.hpp"
#include "thumbnail_cache.hpp"

#include <QImage>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTest>
#include <QUrl>

#include <memory>

using namespace tyli;
using namespace tyli::test;

class TestMediaModels : public QObject {
    Q_OBJECT

private:
    std::unique_ptr<QTemporaryDir> data_;
    std::unique_ptr<QTemporaryDir> media_;
    std::unique_ptr<LibraryBridge> bridge_;

    QString media(const QString& relative) const { return media_->filePath(relative); }

    static void waitForReload() { QTest::qWait(1); QCoreApplication::processEvents(); }

    static void fetchAll(QAbstractListModel& model) {
        while (model.canFetchMore({})) model.fetchMore({});
    }

    // Albums: "A/One" (3 tracks, 16/44.1) and its 24/96 copy, "A/Two", "B/Three"; tracks seeded.
    void seedMusic() {
        for (const QString& album : {"A/One", "A/One [hi-res]", "A/Two", "B/Three"}) {
            for (int i = 1; i <= 3; ++i) writeFile(media(QStringLiteral("Music/%1/%2.flac").arg(album).arg(i)));
        }
        QImage cover(40, 40, QImage::Format_RGB32);
        cover.fill(Qt::red);
        QVERIFY(cover.save(media("Music/A/Two/folder.png")));
        QVERIFY(addFolder(*bridge_, categoryId(*bridge_, "Music"), media("Music"), "Music").first);
        QVERIFY(!sync(*bridge_, -1).isEmpty());
        seedAudio(bridge_->databasePath(), [](const hoardor::file::Entry& e) -> std::optional<hoardor::audio::TrackInfo> {
            const QStringList parts = QString::fromStdString(e.relative_path).split('/');
            const bool hires = parts[1].endsWith("[hi-res]");
            const std::string album = QString(parts[1]).remove(" [hi-res]").toStdString();
            const int n = QFileInfo(parts[2]).baseName().toInt();
            return track(parts[0].toStdString(), album, 1, n, "Song " + std::to_string(n), album == "Three" ? 1990 : 2000 + n,
                         hires ? 24 : 16, hires ? 96000 : 44100);
        });
        emit bridge_->contentChanged();
    }

private slots:
    void init() {
        QStandardPaths::setTestModeEnabled(true);
        data_ = std::make_unique<QTemporaryDir>();
        media_ = std::make_unique<QTemporaryDir>();
        bridge_ = std::make_unique<LibraryBridge>(data_->path());
        LibraryBridge::setInstance(bridge_.get());
        QVERIFY(bridge_->ready());
    }

    void cleanup() {
        bridge_.reset();
        media_.reset();
        data_.reset();
    }

    void albumsAsGroupsWithTotals() {
        seedMusic();
        GroupModel albums;
        albums.setGroupBy({"albumArtist", "album"});
        albums.setFilter({{"category", categoryId(*bridge_, "Music")}});
        waitForReload();
        fetchAll(albums);
        QCOMPARE(albums.rowCount(), 3);  // the two copies of "One" are one group
        QCOMPARE(albums.total(), 3);
        QCOMPARE(albums.get(0).value("values").toStringList(), (QStringList{"A", "One"}));
        QCOMPARE(albums.get(0).value("count").toInt(), 6);

        albums.setGroupBy({"albumArtist"});
        waitForReload();
        QCOMPARE(albums.total(), 2);

        // Year, newest first.
        albums.setGroupBy({"albumArtist", "album"});
        albums.setOrder("year");
        albums.setDescending(true);
        waitForReload();
        fetchAll(albums);
        QCOMPARE(albums.get(2).value("values").toStringList(), (QStringList{"B", "Three"}));

        // A view fetches the first page inside the reset (as GridView does); a small list then
        // ends right away, and the total must still be counted.
        GroupModel viewed;
        connect(&viewed, &QAbstractItemModel::modelReset, &viewed, [&viewed] { viewed.fetchMore({}); });
        viewed.setGroupBy({"albumArtist", "album"});
        waitForReload();
        QCOMPARE(viewed.rowCount(), 3);
        QCOMPARE(viewed.total(), 3);

        // A bad field is an error, not a crash.
        albums.setGroupBy({"nonsense"});
        waitForReload();
        QCOMPARE(albums.rowCount(), 0);
        QVERIFY(!albums.error().isEmpty());
    }

    void newContentIsInsertedInPlace() {
        // Two albums read so far; a third appears mid-pass, and one gets more tracks.
        for (const QString& f : {"A/One/1.flac", "A/One/2.flac", "C/Three/1.flac", "B/Two/1.flac"}) writeFile(media("Music/" + f));
        QVERIFY(addFolder(*bridge_, categoryId(*bridge_, "Music"), media("Music"), "Music").first);
        QVERIFY(!sync(*bridge_, -1).isEmpty());
        const auto seedOnly = [&](const QStringList& paths) {
            seedAudio(bridge_->databasePath(), [&](const hoardor::file::Entry& e) -> std::optional<hoardor::audio::TrackInfo> {
                const QString path = QString::fromStdString(e.relative_path);
                if (!paths.contains(path)) return std::nullopt;
                const QStringList parts = path.split('/');
                return track(parts[0].toStdString(), parts[1].toStdString(), 1, QFileInfo(parts[2]).baseName().toInt(), "s", 2000);
            });
        };
        seedOnly({"A/One/1.flac", "C/Three/1.flac"});
        GroupModel albums;
        albums.setGroupBy({"albumArtist", "album"});
        waitForReload();
        QCOMPARE(albums.rowCount(), 2);

        QSignalSpy resets(&albums, &QAbstractItemModel::modelReset);
        QSignalSpy inserted(&albums, &QAbstractItemModel::rowsInserted);
        QSignalSpy changed(&albums, &QAbstractItemModel::dataChanged);
        seedOnly({"A/One/2.flac", "B/Two/1.flac"});
        emit bridge_->contentChanged();
        QCOMPARE(resets.count(), 0);  // no reset: the view keeps its scroll position
        QCOMPARE(inserted.count(), 1);
        QCOMPARE(albums.rowCount(), 3);
        QCOMPARE(albums.get(1).value("values").toStringList(), (QStringList{"B", "Two"}));  // in its sorted place
        QCOMPARE(albums.get(0).value("count").toInt(), 2);                                // "One" grew
        QVERIFY(changed.count() >= 1);
        QCOMPARE(albums.total(), 3);
    }

    void groupsPageAsTheViewScrolls() {
        for (int i = 0; i < 300; ++i) writeFile(media(QStringLiteral("Music/a%1/1.flac").arg(i)));
        QVERIFY(addFolder(*bridge_, categoryId(*bridge_, "Music"), media("Music"), "Music").first);
        QVERIFY(!sync(*bridge_, -1).isEmpty());
        seedAudio(bridge_->databasePath(), [](const hoardor::file::Entry& e) -> std::optional<hoardor::audio::TrackInfo> {
            return track("X", QString::fromStdString(e.relative_path).section('/', 0, 0).toStdString(), 1, 1, "s", 2000);
        });
        GroupModel albums;
        albums.setGroupBy({"albumArtist", "album"});
        waitForReload();
        QCOMPARE(albums.rowCount(), GroupModel::PageSize);
        QCOMPARE(albums.total(), 300);
        QVERIFY(albums.canFetchMore({}));
        fetchAll(albums);
        QCOMPARE(albums.rowCount(), 300);
        // Natural order of the album names: a0, a1, a2 … a10.
        QCOMPARE(albums.get(10).value("values").toStringList().value(1), QStringLiteral("a10"));
    }

    void tracksOfAnAlbumAndReloadOnNewContent() {
        seedMusic();
        ItemModel tracks;
        tracks.setFilter({{"albumArtist", "a"}, {"album", "two"}});  // case doesn't matter
        tracks.setOrder({"disc", "track"});
        waitForReload();
        QCOMPARE(tracks.rowCount(), 3);
        QCOMPARE(tracks.get(0).value("title").toString(), QStringLiteral("Song 1"));
        QCOMPARE(tracks.get(2).value("track").toInt(), 3);
        QVERIFY(tracks.get(0).value("online").toBool());

        tracks.setOrder({"-track"});
        waitForReload();
        QCOMPARE(tracks.get(0).value("track").toInt(), 3);

        QSignalSpy reset(&tracks, &QAbstractItemModel::modelReset);
        emit bridge_->contentChanged();
        QTRY_VERIFY(reset.count() >= 1);
    }

    void albumCopiesBestFirst() {
        seedMusic();
        const QVariantList copies = bridge_->albumCopies("A", "One");
        QCOMPARE(copies.size(), 2);
        QCOMPARE(copies[0].toMap().value("label").toString(), QStringLiteral("24/96 FLAC"));
        QCOMPARE(copies[0].toMap().value("tracks").toList().size(), 3);
        QVERIFY(copies[1].toMap().value("complete").toBool());
        QVERIFY(bridge_->albumCopies("A", "Nope").isEmpty());
    }

    void videoCopiesShowsAndDefaultViews() {
        writeFile(media("Films/Arrival/a.mkv"));
        writeFile(media("Films/Arrival/b.mkv"));
        writeFile(media("Series/Dark/S01E01.mkv"));
        writeFile(media("Series/Dark/S01E02.mkv"));
        const int movies = categoryId(*bridge_, "Movies"), shows = categoryId(*bridge_, "Shows");
        QVERIFY(addFolder(*bridge_, movies, media("Films"), "Films").first);
        QVERIFY(addFolder(*bridge_, shows, media("Series"), "Series").first);
        QVERIFY(!sync(*bridge_, -1).isEmpty());
        seedVideo(bridge_->databasePath(), [](const hoardor::file::Entry& e, const std::vector<hoardor::file::Entry>&)
                                               -> std::optional<std::pair<hoardor::video::VideoInfo, qint64>> {
            hoardor::video::VideoInfo v;
            const QString path = QString::fromStdString(e.relative_path);
            v.height = path.endsWith("b.mkv") ? 2160 : 1080;
            v.video_codec = "hevc";
            if (path.startsWith("Arrival")) {
                v.title = "Arrival";
                v.year = 2016;
            } else {
                v.type = hoardor::video::Type::Episode;
                v.show = "Dark";
                v.season = 1;
                v.episode = path.endsWith("E02.mkv") ? 2 : 1;
                v.title = "Ep";
            }
            return std::make_pair(v, qint64(0));
        });
        const QVariantList copies = bridge_->movieCopies("Arrival", 2016);
        QCOMPARE(copies.size(), 2);
        QVERIFY(copies[0].toMap().value("label").toString().startsWith("2160p"));
        const QVariantMap show = bridge_->showCopies("Dark");
        QCOMPARE(show.value("episodes").toList().size(), 2);
        QCOMPARE(show.value("qualities").toList().size(), 1);

        QCOMPARE(bridge_->categoryView(categoryId(*bridge_, "Music")), QStringLiteral("music"));
        QCOMPARE(bridge_->categoryView(movies), QStringLiteral("movies"));
        QCOMPARE(bridge_->categoryView(shows), QStringLiteral("shows"));
        QVERIFY(bridge_->addCategory("Books", {"text"}));  // not seeded any more (hoardor file migration 3)
        QCOMPARE(bridge_->categoryView(categoryId(*bridge_, "Books")), QStringLiteral("none"));
        // The user's choice wins, and "" goes back to automatic.
        QSignalSpy changed(bridge_.get(), &LibraryBridge::categoryViewsChanged);
        bridge_->setCategoryView(movies, "shows");
        QCOMPARE(bridge_->categoryView(movies), QStringLiteral("shows"));
        bridge_->setCategoryView(movies, "");
        QCOMPARE(bridge_->categoryView(movies), QStringLiteral("movies"));
        QCOMPARE(changed.count(), 2);

        GroupModel cards;
        cards.setEngine("video");
        cards.setGroupBy({"title", "year"});
        cards.setFilter({{"type", 1}});
        waitForReload();
        QCOMPARE(cards.rowCount(), 1);  // one card for both copies
        QCOMPARE(cards.get(0).value("maxHeight").toInt(), 2160);
    }

    void thumbnailsFromImagesNextToTracks() {
        seedMusic();
        const QString folder = data_->filePath("thumbnails");
        const auto first = ThumbnailCache::build(folder, bridge_->databasePath());
        QCOMPARE(first.made, 1);    // A/Two has folder.png; the others have no art
        QCOMPARE(first.failed, 2);
        const auto again = ThumbnailCache::build(folder, bridge_->databasePath());
        QCOMPARE(again.made, 0);    // nothing redone
        QCOMPARE(QDir(folder).entryList(QDir::Files).size(), 1);
        const QImage thumbnail(QDir(folder).filePath(QDir(folder).entryList(QDir::Files).first()));
        QVERIFY(!thumbnail.isNull());
        QVERIFY(thumbnail.width() <= ThumbnailCache::Width);

        GroupModel albums;
        albums.setGroupBy({"albumArtist", "album"});
        waitForReload();
        fetchAll(albums);
        emit bridge_->thumbnails()->changed();
        int withArt = 0;
        for (int i = 0; i < albums.rowCount(); ++i) withArt += !albums.get(i).value("thumbnail").toString().isEmpty();
        QCOMPARE(withArt, 1);

        // An orphan is removed.
        QFile stray(QDir(folder).filePath("999-1.jpg"));
        QVERIFY(stray.open(QIODevice::WriteOnly));
        stray.close();
        QCOMPARE(ThumbnailCache::build(folder, bridge_->databasePath()).removed, 1);
    }

    void scrollSpeedIsAPreferenceAppliedAtOnce() {
        QCOMPARE(bridge_->scrollSpeed(), DefaultScrollSpeedPercent);
        QCOMPARE(wheelSpeed(), 2.0);
        QSignalSpy changed(bridge_.get(), &LibraryBridge::scrollSpeedChanged);
        bridge_->setScrollSpeed(150);
        QCOMPARE(wheelSpeed(), 1.5);
        QCOMPARE(changed.count(), 1);
        bridge_->setScrollSpeed(9000);  // clamped
        QCOMPARE(bridge_->scrollSpeed(), 500);
        // A new bridge (the next start) applies the stored value.
        bridge_.reset();
        setWheelSpeed(1.0);
        bridge_ = std::make_unique<LibraryBridge>(data_->path());
        QCOMPARE(wheelSpeed(), 5.0);
        bridge_->setScrollSpeed(DefaultScrollSpeedPercent);
    }

    // Preferences are an INI file next to library.db (QSettings() saved nothing on Windows),
    // survive a restart, and come back with the fallback's type.
    void preferencesPersistInTheDataFolderWithTheirTypes() {
        bridge_->setPreference("sort/music/added", false);
        bridge_->setPreference("sort/music", "year");
        bridge_->setCategoryView(categoryId(*bridge_, "Movies"), "shows");
        QVERIFY(QFile::exists(data_->filePath("preferences.ini")));
        bridge_.reset();
        bridge_ = std::make_unique<LibraryBridge>(data_->path());  // the next start
        const QVariant descending = bridge_->preference("sort/music/added", true);
        QCOMPARE(descending.metaType(), QMetaType::fromType<bool>());
        QCOMPARE(descending.toBool(), false);
        QCOMPARE(bridge_->preference("sort/music", "added").toString(), QStringLiteral("year"));
        QCOMPARE(bridge_->categoryView(categoryId(*bridge_, "Movies")), QStringLiteral("shows"));
    }

    void trackColumnsArePreferences() {
        QCOMPARE(bridge_->trackColumns(), (QStringList{"length", "bitrate"}));
        QSignalSpy changed(bridge_.get(), &LibraryBridge::trackColumnsChanged);
        bridge_->setTrackColumns({"size", "length", "unknown"});
        QCOMPARE(bridge_->trackColumns(), (QStringList{"length", "size"}));  // canonical order, known names only
        QCOMPARE(changed.count(), 1);
        bridge_->setTrackColumns({"length", "bitrate"});
    }
};

QTEST_MAIN(TestMediaModels)
#include "tst_media_models.moc"
