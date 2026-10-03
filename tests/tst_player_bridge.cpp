// PlayerBridge and QueueModel over a real library, with hoardor's player headless
// (TYLI_PLAYER_OUTPUTS=null): what plays is described, skipped files make one status line,
// likes and states come back, and the queue model follows the player.

#include "library_bridge.hpp"
#include "player_bridge.hpp"
#include "queue_model.hpp"
#include "test_support.hpp"

#include <QProcess>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTest>

#include <memory>

using namespace tyli;
using namespace tyli::test;

class TestPlayerBridge : public QObject {
    Q_OBJECT

private:
    std::unique_ptr<QTemporaryDir> data_;
    std::unique_ptr<QTemporaryDir> media_;
    std::unique_ptr<LibraryBridge> library_;
    std::unique_ptr<PlayerBridge> player_;
    QMap<QString, qint64> ids_;   // file name -> entry id

    // Three tracks of one album, as the metadata pass would have stored them. With `real`,
    // they're short tones the ffmpeg tool makes; otherwise junk mpv can't play.
    bool seed(bool real) {
        for (int i = 1; i <= 3; ++i) {
            const QString file = media_->filePath(QStringLiteral("Music/Band/Album/%1.flac").arg(i));
            if (real) {
                const QString ffmpeg = QStandardPaths::findExecutable(QStringLiteral("ffmpeg"));
                if (ffmpeg.isEmpty()) return false;
                QDir().mkpath(QFileInfo(file).absolutePath());
                if (QProcess::execute(ffmpeg, {"-y", "-loglevel", "error", "-f", "lavfi", "-t", "3", "-i", "sine", "-c:a", "flac", file}) != 0) {
                    return false;
                }
            } else {
                writeFile(file);
            }
        }
        if (!addFolder(*library_, categoryId(*library_, "Music"), media_->filePath("Music"), "Music").first) return false;
        if (sync(*library_, -1).isEmpty()) return false;
        seedAudio(library_->databasePath(), [this](const hoardor::file::Entry& e) -> std::optional<hoardor::audio::TrackInfo> {
            const int n = QFileInfo(QString::fromStdString(e.relative_path)).baseName().toInt();
            ids_[QFileInfo(QString::fromStdString(e.relative_path)).fileName()] = e.id;
            return track("Band", "Album", 1, n, "Song " + std::to_string(n), 2001);
        });
        return true;
    }

    QVariantList album() const { return {ids_["1.flac"], ids_["2.flac"], ids_["3.flac"]}; }

private slots:
    void init() {
        qputenv("TYLI_PLAYER_OUTPUTS", "null");
        data_ = std::make_unique<QTemporaryDir>();
        media_ = std::make_unique<QTemporaryDir>();
        library_ = std::make_unique<LibraryBridge>(data_->path());
        LibraryBridge::setInstance(library_.get());
        QVERIFY(library_->ready());
        // The default settle window keeps these new files from being read for tags (which would
        // replace the made-up ones); playing doesn't need them settled.
        player_ = std::make_unique<PlayerBridge>(library_->databasePath());
        PlayerBridge::setInstance(player_.get());
        QVERIFY(player_->ready());
        ids_.clear();
    }

    void cleanup() {
        player_.reset();
        library_.reset();
        media_.reset();
        data_.reset();
    }

    void playsDescribesAndQueues() {
        if (!seed(true)) QSKIP("the ffmpeg tool isn't installed");
        QueueModel queue;
        player_->playNow(album(), 1);
        QTRY_COMPARE(player_->state(), QStringLiteral("playing"));
        QCOMPARE(player_->entryId(), ids_["2.flac"]);
        QCOMPARE(player_->title(), QStringLiteral("Song 2"));
        QCOMPARE(player_->subtitle(), QStringLiteral("Band · Album"));
        QCOMPARE(player_->link().value("page").toString(), QStringLiteral("album"));
        QCOMPARE(player_->link().value("params").toMap().value("album").toString(), QStringLiteral("Album"));
        QVERIFY(player_->coverEntry() > 0);   // the album's cover track, for the thumbnail
        QTRY_COMPARE(queue.rowCount(), 3);
        QCOMPARE(queue.data(queue.index(1), QueueModel::PlayingRole).toBool(), true);
        QCOMPARE(queue.data(queue.index(0), QueueModel::PlayedRole).toBool(), true);
        QCOMPARE(queue.data(queue.index(2), QueueModel::TitleRole).toString(), QStringLiteral("Song 3"));
        QCOMPARE(queue.remainingDuration(), qint64(241'000));   // Song 3, as seeded

        player_->add({ids_["1.flac"]});
        QTRY_COMPARE(queue.rowCount(), 4);
        player_->clear();
        QTRY_VERIFY(!player_->active());
        QTRY_COMPARE(queue.rowCount(), 0);
    }

    void skippedFilesMakeOneStatusLine() {
        QVERIFY(seed(false));   // junk: mpv can't play any of them
        QSignalSpy notices(library_.get(), &LibraryBridge::notice);
        player_->playNow(album(), 0);
        QTRY_COMPARE(notices.count(), 1);
        QTest::qWait(600);
        QCOMPARE(notices.count(), 1);   // one line for the three
        const QString line = notices.first().at(0).toString();
        QVERIFY2(line.startsWith(QStringLiteral("3 files skipped")), qPrintable(line));
        QTRY_VERIFY(!player_->active());
    }

    void likesAndStatesComeBack() {
        QVERIFY(seed(false));
        QSignalSpy states(player_.get(), &PlayerBridge::statesChanged);
        const qint64 one = ids_["1.flac"];
        QVERIFY(!player_->isLiked(one));
        const int before = player_->revision();
        player_->setLiked(one, true);
        QVERIFY(player_->isLiked(one));
        QCOMPARE(states.count(), 1);
        QVERIFY(player_->revision() > before);   // bindings that read it re-evaluate
        const QVariantList list = player_->itemStates({one, ids_["2.flac"]});
        QCOMPARE(list.size(), 2);
        QCOMPARE(list[0].toMap().value("liked").toBool(), true);
        QCOMPARE(list[1].toMap().value("liked").toBool(), false);
        QCOMPARE(list[1].toMap().value("entryId").toLongLong(), ids_["2.flac"]);
        player_->setLiked(one, false);
        QVERIFY(!player_->isLiked(one));
        // A restart keeps likes (another bridge on the same library).
        player_->setLiked(one, true);
        player_.reset();
        player_ = std::make_unique<PlayerBridge>(library_->databasePath());
        PlayerBridge::setInstance(player_.get());
        QVERIFY(player_->isLiked(one));
    }
};

QTEST_MAIN(TestPlayerBridge)
#include "tst_player_bridge.moc"
