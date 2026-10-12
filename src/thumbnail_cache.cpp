#include "thumbnail_cache.hpp"

#include <hoardor/audio/audio.hpp>
#include <hoardor/db/database.hpp>
#include <hoardor/video/video.hpp>

#include <QBuffer>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QImage>
#include <QImageReader>
#include <QSaveFile>
#include <QSet>
#include <QUrl>
#include <QtConcurrent/QtConcurrentRun>

#include <filesystem>
#include <functional>
#include <map>

namespace tyli {

namespace file = hoardor::file;
namespace audio = hoardor::audio;
namespace video = hoardor::video;

ThumbnailCache::ThumbnailCache(const QString& folder, const QString& databasePath, file::Library* uiLibrary, QObject* parent)
    : QObject(parent), folder_(folder), databasePath_(databasePath), uiLibrary_(uiLibrary) {
    QDir().mkpath(folder_);
    connect(this, &ThumbnailCache::changed, this, [this] { known_.clear(); });
}

QString ThumbnailCache::fileName(qint64 entryId, qint64 mtimeNs) {
    return QStringLiteral("%1-%2.jpg").arg(entryId).arg(mtimeNs);
}

QString ThumbnailCache::url(qint64 entryId) {
    if (entryId <= 0 || !uiLibrary_) return {};
    const auto it = known_.constFind(entryId);
    if (it != known_.cend()) return *it;
    QString result;
    if (auto entry = uiLibrary_->entry(entryId)) {
        const QString path = QDir(folder_).filePath(fileName(entryId, entry->mtime_ns));
        if (QFile::exists(path)) result = QUrl::fromLocalFile(path).toString();
    }
    known_.insert(entryId, result);
    return result;
}

void ThumbnailCache::rebuild(bool frames) {
    if (running_) {
        again_ = true;
        againFrames_ = againFrames_ || frames;
        return;
    }
    running_ = true;
    emit runningChanged();
    Options options;
    options.frames = frames;
    options.plexFolder = qEnvironmentVariable("TYLI_PLEX_DIR");
    options.failedFrames = failedFrames_;
    options.plexMissed = plexMissed_;
    auto future = QtConcurrent::run([folder = folder_, path = databasePath_, options] { return build(folder, path, options); });
    auto* watcher = new QFutureWatcher<Result>(this);
    connect(watcher, &QFutureWatcherBase::finished, this, [this, watcher] {
        const Result r = watcher->result();
        watcher->deleteLater();
        running_ = false;
        failedFrames_.unite(r.failedFrames);
        plexMissed_.unite(r.plexMissed);
        emit runningChanged();
        if (r.made > 0 || r.removed > 0) emit changed();
        if (again_) {
            again_ = false;
            const bool frames = std::exchange(againFrames_, false);
            rebuild(frames);
        }
    });
    watcher->setFuture(future);
}

namespace {

QByteArray readFile(const std::filesystem::path& path) {
    QFile f(QString::fromStdU16String(path.u16string()));
    return f.open(QIODevice::ReadOnly) ? f.readAll() : QByteArray();
}

QByteArray bytes(const std::vector<std::byte>& data) {
    return QByteArray(reinterpret_cast<const char*>(data.data()), qsizetype(data.size()));
}

// Scales without decoding at full size (QImageReader::setScaledSize), then saves a JPEG.
bool writeThumbnail(const QByteArray& image, const QString& target) {
    QBuffer buffer;
    buffer.setData(image);
    buffer.open(QIODevice::ReadOnly);
    QImageReader reader(&buffer);
    const QSize size = reader.size();
    if (size.isValid() && size.width() > ThumbnailCache::Width) {
        reader.setScaledSize(QSize(ThumbnailCache::Width, qMax(1, size.height() * ThumbnailCache::Width / size.width())));
    }
    QImage scaled = reader.read();
    if (scaled.isNull()) return false;
    if (scaled.width() > ThumbnailCache::Width) scaled = scaled.scaledToWidth(ThumbnailCache::Width, Qt::SmoothTransformation);
    QSaveFile out(target);  // written in full or not at all
    if (!out.open(QIODevice::WriteOnly)) return false;
    if (!scaled.convertToFormat(QImage::Format_RGB888).save(&out, "JPG", 85)) return false;
    return out.commit();
}

// A frame (RGB) as a JPEG, for writeThumbnail.
QByteArray jpeg(const hoardor::video::Frame& frame) {
    const QImage image(reinterpret_cast<const uchar*>(frame.rgb.data()), frame.width, frame.height, frame.width * 3,
                       QImage::Format_RGB888);
    QByteArray out;
    QBuffer buffer(&out);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "JPG", 92);
    return out;
}

}

ThumbnailCache::Result ThumbnailCache::build(const QString& folder, const QString& databasePath) {
    return build(folder, databasePath, Options{});
}

ThumbnailCache::Result ThumbnailCache::build(const QString& folder, const QString& databasePath, const Options& options) {
    Result result;
    auto database = hoardor::db::Database::open(databasePath.toStdU16String());
    if (!database) return result;
    auto files = file::Library::open(*database);
    auto tracks = audio::Library::open(*database);
    auto videos = video::Library::open(*database);
    if (!files || !tracks || !videos) return result;
    QDir().mkpath(folder);

    std::map<file::RootId, std::filesystem::path> online;
    if (auto roots = files->roots()) {
        for (const auto& r : *roots) {
            if (r.status == file::RootStatus::Online) online[r.id] = std::filesystem::path(QString::fromStdString(r.path).toStdU16String());
        }
    }
    const auto pathOf = [&](const file::Entry& e) -> std::optional<std::filesystem::path> {
        const auto it = online.find(e.root_id);
        if (it == online.end()) return std::nullopt;
        return it->second / std::filesystem::path(QString::fromStdString(e.relative_path).toStdU16String());
    };

    QSet<QString> keep;
    bool deferred = false;  // set by a `load` that can't try now (not a failure)
    // `source`: the entry the thumbnail is named after; `load` reads the image bytes (drive).
    const auto make = [&](file::EntryId source, const std::function<QByteArray(const file::Entry&, const std::filesystem::path&)>& load) {
        if (source <= 0) return;
        auto entry = files->entry(source);
        if (!entry) return;
        const QString name = fileName(source, entry->mtime_ns);
        if (keep.contains(name)) return;
        keep.insert(name);
        const QString target = QDir(folder).filePath(name);
        if (QFile::exists(target)) return;
        const auto path = pathOf(*entry);
        if (!path) return;  // offline: try again after a later sync
        deferred = false;
        const QByteArray image = load(*entry, *path);
        if (deferred) return;
        if (!image.isEmpty() && writeThumbnail(image, target)) ++result.made;
        else ++result.failed;
    };

    // Album art: the embedded picture of the group's cover track; else an image next to any of
    // the album's tracks (copies of an album can sit in different folders, only one with art).
    const QStringList cover_names{QStringLiteral("cover"), QStringLiteral("folder"), QStringLiteral("front"),
                                  QStringLiteral("albumart"), QStringLiteral("album")};
    const auto image_next_to = [&](file::EntryId track) -> std::optional<std::filesystem::path> {
        auto companions = files->companions(track);
        if (!companions) return std::nullopt;
        const file::Entry* best = nullptr;
        int best_rank = 1000;
        for (const auto& c : *companions) {
            if (c.kind != file::FileKind::Image) continue;
            const QString stem = QFileInfo(QString::fromStdString(c.relative_path)).completeBaseName().toLower();
            const int rank = cover_names.contains(stem) ? int(cover_names.indexOf(stem)) : 100;
            if (rank < best_rank) {
                best = &c;
                best_rank = rank;
            }
        }
        return best ? pathOf(*best) : std::nullopt;
    };
    const std::vector<audio::Field> album{audio::Field::AlbumArtist, audio::Field::Album};
    for (std::optional<hoardor::core::Cursor> after;;) {
        auto page = tracks->groups(album, {}, audio::GroupOrder::Values, false, after, 500);
        if (!page) break;
        for (const auto& g : page->items) {
            make(g.cover_entry, [&](const file::Entry& entry, const std::filesystem::path& path) -> QByteArray {
                if (auto t = tracks->track(entry.id); t && t->has_embedded_cover) {
                    if (auto picture = audio::embedded_cover(path); picture && !picture->empty()) return bytes(*picture);
                }
                if (auto image = image_next_to(entry.id)) return readFile(*image);
                // The album's other tracks, one per folder, until one has an image.
                const audio::Filter one{{{audio::Field::AlbumArtist, audio::Value{g.values[0]}}, {audio::Field::Album, audio::Value{g.values[1]}}}};
                auto others = tracks->tracks(one, {}, std::nullopt, 200);
                if (!others) return {};
                QSet<QString> folders;
                for (const auto& t : others->items) {
                    auto e = files->entry(t.entry_id);
                    if (!e) continue;
                    const QString folder = QFileInfo(QString::fromStdString(e->relative_path)).path();
                    if (folders.contains(folder)) continue;
                    folders.insert(folder);
                    if (auto image = image_next_to(t.entry_id)) return readFile(*image);
                }
                return {};
            });
        }
        if (!page->next) break;
        after = page->next;
    }

    // Posters: a companion image file, else a picture inside the video, else (below) Plex's or a frame.
    const auto image_file = [&](const file::Entry&, const std::filesystem::path& path) { return readFile(path); };
    const auto embedded = [&](const file::Entry&, const std::filesystem::path& path) -> QByteArray {
        auto picture = video::embedded_poster(path);
        return picture ? bytes(*picture) : QByteArray();
    };
    // Nothing of its own: Plex's poster (internal storage), else a frame from the video (the
    // drive; only in passes that allow it). Plex's database is read once, when first needed.
    std::optional<video::PlexPosters> plex;
    bool plexOpened = false;
    const auto plexOrFrame = [&](const file::Entry& entry, const std::filesystem::path& path) -> QByteArray {
        if (!options.frames && options.plexMissed.contains(entry.id)) {  // asked Plex already; frames come later
            deferred = true;
            return {};
        }
        if (!plexOpened) {
            plexOpened = true;
            std::optional<std::filesystem::path> where;
            if (!options.plexFolder.isEmpty()) where = std::filesystem::path(options.plexFolder.toStdU16String());
            else where = video::PlexPosters::find_folder();
            if (where) {
                if (auto opened = video::PlexPosters::open(*where)) plex.emplace(std::move(*opened));
            }
        }
        if (plex) {
            if (auto image = plex->poster(path)) {
                QByteArray data = readFile(*image);
                if (!data.isEmpty()) {
                    ++result.fromPlex;
                    return data;
                }
            }
        }
        if (!options.frames) {
            result.plexMissed.insert(entry.id);
            deferred = true;
            return {};
        }
        if (options.failedFrames.contains(entry.id)) {
            deferred = true;
            return {};
        }
        auto frame = video::grab_frame(path);
        if (!frame) {
            result.failedFrames.insert(entry.id);
            return {};
        }
        ++result.fromFrames;
        return jpeg(*frame);
    };
    // Movies and episodes only: extras (featurettes, trailers, …) get no poster, so a frame is
    // never grabbed for each of them (hoardor features/shows_and_extras.md).
    const std::vector<video::Field> titled{video::Field::Title, video::Field::Year}, shown{video::Field::Show};
    const auto of_type = [](std::int64_t type) { return video::Filter{{{video::Field::Type, video::Value{type}}}}; };
    const std::pair<const std::vector<video::Field>*, video::Filter> passes[] = {
        {&titled, of_type(1)}, {&titled, of_type(2)}, {&shown, of_type(2)}};
    for (const auto& [by, filter] : passes) {
        for (std::optional<hoardor::core::Cursor> after;;) {
            auto page = videos->groups(*by, filter, video::GroupOrder::Values, false, after, 500);
            if (!page) break;
            for (const auto& g : page->items) {
                if (g.poster_entry) make(g.poster_entry, image_file);
                else if (g.embedded_poster_entry) make(g.embedded_poster_entry, embedded);
                else make(g.first_entry, plexOrFrame);
            }
            if (!page->next) break;
            after = page->next;
        }
    }

    // Thumbnails nothing refers to any more.
    for (const QString& name : QDir(folder).entryList({QStringLiteral("*.jpg")}, QDir::Files)) {
        if (!keep.contains(name) && QFile::remove(QDir(folder).filePath(name))) ++result.removed;
    }
    return result;
}

}
