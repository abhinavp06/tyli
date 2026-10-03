#pragma once

#include "library_bridge.hpp"

#include <hoardor/audio/audio.hpp>
#include <hoardor/file/library.hpp>
#include <hoardor/video/video.hpp>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSignalSpy>
#include <QTest>

#include <functional>
#include <optional>

// Helpers shared by the suites: media trees on disk, and waiting for the bridge's
// background work.
namespace tyli::test {

inline void writeFile(const QString& path, qint64 size = 16) {
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile file(path);
    QVERIFY2(file.open(QIODevice::WriteOnly), qPrintable(path));
    file.write(QByteArray(int(size), 'x'));
}

// `count` .flac files under `root`, 100 per sub-folder.
inline void writeTree(const QString& root, int count) {
    for (int i = 0; i < count; ++i) {
        writeFile(QStringLiteral("%1/album %2/track %3.flac").arg(root).arg(i / 100).arg(i, 5, 10, QLatin1Char('0')));
    }
}

inline int categoryId(LibraryBridge& bridge, const QString& name) {
    for (const QVariant& c : bridge.categories()) {
        if (c.toMap().value("name").toString() == name) return c.toMap().value("id").toInt();
    }
    return -1;
}

inline QVariantMap folder(LibraryBridge& bridge, const QString& name) {
    for (const QVariant& f : bridge.folders()) {
        if (f.toMap().value("name").toString() == name) return f.toMap();
    }
    return {};
}

// addFolder, waiting for its result. Returns {ok, message}.
inline std::pair<bool, QString> addFolder(LibraryBridge& bridge, int categoryId, const QString& path,
                                          const QString& name = {}, bool useMarker = true) {
    QSignalSpy done(&bridge, &LibraryBridge::folderOperationFinished);
    bridge.addFolder(categoryId, path, name, useMarker);
    if (!done.wait(10000)) return {false, QStringLiteral("timed out")};
    return {done.first().at(0).toBool(), done.first().at(1).toString()};
}

// requestSync, waiting until the bridge is idle again. Returns the report.
inline QVariantMap sync(LibraryBridge& bridge, int categoryId) {
    QSignalSpy finished(&bridge, &LibraryBridge::syncFinished);
    if (!bridge.requestSync(categoryId)) return {};
    if (!finished.wait(20000)) return {};
    if (!QTest::qWaitFor([&] { return !bridge.syncing(); }, 5000)) return {};
    return finished.first().at(0).toMap();
}

inline QVariantMap folderReport(const QVariantMap& report, int index = 0) {
    return report.value("folders").toList().value(index).toMap();
}

// Stores made-up metadata for the library's files, on a connection of its own, as the
// metadata pass would (the test files are empty, so there's nothing real to read).
// `make` returns the info for an entry, or nullopt to leave it unread.
inline void seedAudio(const QString& databasePath,
                      const std::function<std::optional<hoardor::audio::TrackInfo>(const hoardor::file::Entry&)>& make) {
    auto db = hoardor::db::Database::open(databasePath.toStdU16String());
    QVERIFY(db.has_value());
    auto files = hoardor::file::Library::open(*db);
    auto tracks = hoardor::audio::Library::open(*db);
    QVERIFY(files && tracks);
    const auto roots = files->roots().value();  // kept alive: range-for over a temporary's value() dangles in GCC 13
    for (const auto& root : roots) {
        for (hoardor::file::EntryId after = 0;;) {
            const auto page = files->entries(root.id, after, 500).value();
            if (page.empty()) break;
            for (const auto& e : page) {
                after = e.id;
                if (e.kind != hoardor::file::FileKind::Audio) continue;
                if (auto info = make(e)) QVERIFY(tracks->store(e.id, std::int64_t(e.size), e.mtime_ns, *info));
            }
        }
    }
}

// `make(entry, companions)`: the info and the poster entry (0: none), or nullopt.
inline void seedVideo(const QString& databasePath,
                      const std::function<std::optional<std::pair<hoardor::video::VideoInfo, qint64>>(
                          const hoardor::file::Entry&, const std::vector<hoardor::file::Entry>&)>& make) {
    auto db = hoardor::db::Database::open(databasePath.toStdU16String());
    QVERIFY(db.has_value());
    auto files = hoardor::file::Library::open(*db);
    auto videos = hoardor::video::Library::open(*db);
    QVERIFY(files && videos);
    const auto roots = files->roots().value();  // kept alive: range-for over a temporary's value() dangles in GCC 13
    for (const auto& root : roots) {
        for (hoardor::file::EntryId after = 0;;) {
            const auto page = files->entries(root.id, after, 500).value();
            if (page.empty()) break;
            for (const auto& e : page) {
                after = e.id;
                if (e.kind != hoardor::file::FileKind::Video) continue;
                const auto companions = files->companions(e.id, 2).value();
                if (auto made = make(e, companions)) QVERIFY(videos->store(e.id, std::int64_t(e.size), e.mtime_ns, made->first, made->second));
            }
        }
    }
}

inline hoardor::audio::TrackInfo track(const std::string& albumArtist, const std::string& album, int disc, int number,
                                       const std::string& title, int year, int bitDepth = 16, int sampleRate = 44100) {
    hoardor::audio::TrackInfo t;
    t.album_artist = albumArtist;
    t.artists = {albumArtist};
    t.album = album;
    t.disc = disc;
    t.track = number;
    t.title = title;
    t.year = year;
    t.date = std::to_string(year);
    t.genres = {"Art Rock"};
    t.duration_ms = 241'000;
    t.codec = "flac";
    t.lossless = true;
    t.bit_depth = bitDepth;
    t.sample_rate = sampleRate;
    t.bitrate_kbps = bitDepth > 16 ? 2'500 : 900;
    t.channels = 2;
    return t;
}

}
