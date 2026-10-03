#include "player_bridge.hpp"

#include "copy_grouper.hpp"
#include "library_bridge.hpp"
#include "media_fields.hpp"

#include <hoardor/master/playback.hpp>

#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QQmlEngine>

#include <chrono>

namespace tyli {

namespace audio = hoardor::audio;
namespace video = hoardor::video;
namespace player = hoardor::player;

namespace {

QString qstr(const std::string& s) { return QString::fromStdString(s); }

std::int64_t nowNs() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

std::vector<player::EntryId> toIds(const QVariantList& list) {
    std::vector<player::EntryId> ids;
    ids.reserve(std::size_t(list.size()));
    for (const QVariant& v : list) ids.push_back(v.toLongLong());
    return ids;
}

// "English · TrueHD 7.1 · Commentary", "Français", "next to the file"
QVariantList tracksToList(const std::vector<player::Track>& tracks, bool audio) {
    QVariantList out;
    for (const auto& t : tracks) {
        QStringList parts;
        if (!t.language.empty()) parts << qstr(t.language);
        QString codec = qstr(t.codec);
        if (audio && t.channels > 0) codec += t.channels == 2 ? QStringLiteral(" stereo") : t.channels == 1 ? QStringLiteral(" mono")
                                                              : QStringLiteral(" %1.1").arg(t.channels - 1);
        if (!codec.isEmpty()) parts << codec;
        if (!t.title.empty()) parts << qstr(t.title);
        if (t.external) parts << QStringLiteral("next to the file");
        out << QVariantMap{{"id", t.id}, {"label", parts.isEmpty() ? QStringLiteral("track %1").arg(t.id) : parts.join(QStringLiteral(" · "))},
                           {"selected", t.selected}};
    }
    return out;
}

}  // namespace

PlayerBridge::PlayerBridge(const QString& databasePath, QObject* parent) : QObject(parent) {
    errorDelay_.setSingleShot(true);
    errorDelay_.setInterval(400);
    connect(&errorDelay_, &QTimer::timeout, this, &PlayerBridge::flushErrors);

    const std::filesystem::path file(databasePath.toStdU16String());
    if (auto database = hoardor::db::Database::open(file)) {
        database_.emplace(std::move(*database));
        if (auto items = player::Library::open(*database_)) items_.emplace(std::move(*items));
    }

    // Tests set TYLI_PLAYER_OUTPUTS=null (no sound, no frames); "noaudio" keeps the frames
    // (a check of the picture on a display, without a sound card).
    player::Outputs outputs;
    const QString wanted = qEnvironmentVariable("TYLI_PLAYER_OUTPUTS");
    if (wanted == QLatin1String("null")) outputs = {"null", "null", {}};
    else if (wanted == QLatin1String("noaudio")) outputs = {"null", "libmpv", {}};
    // TYLI_MPV_LOG=1: mpv writes mpv.log next to library.db (which decoder, hardware decoding,
    // scaler, and dithering it used), to diagnose playback on the user's machine.
    if (qEnvironmentVariable("TYLI_MPV_LOG") == QLatin1String("1")) {
        outputs.log_file = std::filesystem::path(QFileInfo(databasePath).absoluteDir().filePath(QStringLiteral("mpv.log")).toStdU16String());
    }

    player::Callbacks callbacks;
    callbacks.status = [this](const player::Status& s) {
        QMetaObject::invokeMethod(this, [this, s] { onStatus(s); }, Qt::QueuedConnection);
    };
    callbacks.position = [this](std::int64_t ms) {
        QMetaObject::invokeMethod(this, [this, ms] {
            position_ = ms;
            emit positionChanged();
        }, Qt::QueuedConnection);
    };
    callbacks.queue = [this] { QMetaObject::invokeMethod(this, [this] { onQueue(); }, Qt::QueuedConnection); };
    callbacks.error = [this](player::EntryId e, const player::Error& error) {
        const QString message = qstr(error.message);
        QMetaObject::invokeMethod(this, [this, e, message] { onError(e, message); }, Qt::QueuedConnection);
    };
    auto started = player::Player::start(file, hoardor::master::file_resolver(file), std::move(callbacks), outputs);
    if (started) {
        player_ = std::move(*started);
        status_ = player_->status();
    } else if (auto* library = LibraryBridge::instance()) {
        emit library->errorOccurred(QStringLiteral("the player couldn't start: ") + qstr(started.error().message));
    }
}

// The player goes first: its thread may still be calling back into this object.
PlayerBridge::~PlayerBridge() { player_.reset(); }

PlayerBridge* PlayerBridge::create(QQmlEngine*, QJSEngine*) {
    QJSEngine::setObjectOwnership(instance_, QJSEngine::CppOwnership);
    return instance_;
}

QString PlayerBridge::state() const {
    switch (status_.state) {
    case player::State::Loading: return QStringLiteral("loading");
    case player::State::Playing: return QStringLiteral("playing");
    case player::State::Paused: return QStringLiteral("paused");
    case player::State::Idle: break;
    }
    return QStringLiteral("idle");
}

QVariantList PlayerBridge::audioTracks() const { return tracksToList(status_.audio, true); }
QVariantList PlayerBridge::subtitleTracks() const { return tracksToList(status_.subtitles, false); }

PlayerBridge::Description PlayerBridge::describe(qint64 entryId) const {
    Description d;
    auto* library = LibraryBridge::instance();
    if (!library || entryId <= 0) return d;
    if (auto* tracks = library->tracks()) {
        if (auto t = tracks->track(entryId)) {
            d.title = qstr(t->title);
            QStringList artists;
            for (const auto& a : t->artists) artists << qstr(a);
            d.subtitle = (artists.isEmpty() ? qstr(t->album_artist) : artists.join(QStringLiteral(", "))) + QStringLiteral(" · ") + qstr(t->album);
            d.duration = t->duration_ms;
            d.artist = qstr(t->album_artist);
            d.album = qstr(t->album);
            d.detail = (t->year > 0 ? QString::number(t->year) + QStringLiteral(" · ") : QString()) + copies::audioLabel(*t);
            // The album's cover is named after its group's cover track (ThumbnailCache).
            const std::vector<audio::Field> by{audio::Field::AlbumArtist, audio::Field::Album};
            const audio::Filter one{{{audio::Field::AlbumArtist, audio::Value{t->album_artist}}, {audio::Field::Album, audio::Value{t->album}}}};
            if (auto page = tracks->groups(by, one, audio::GroupOrder::Values, false, std::nullopt, 1); page && !page->items.empty()) {
                d.coverEntry = page->items.front().cover_entry;
            }
            const int category = library->categoryOfFolder(int(t->root_id));
            d.link = {{"categoryId", category},
                      {"page", QStringLiteral("album")},
                      {"params", QVariantMap{{"albumArtist", qstr(t->album_artist)}, {"album", qstr(t->album)}, {"crumb", qstr(t->album)}}}};
            d.artistLink = {{"categoryId", category},
                            {"page", QStringLiteral("artist")},
                            {"params", QVariantMap{{"name", qstr(t->album_artist)}, {"crumb", qstr(t->album_artist)}}}};
            return d;
        }
    }
    if (auto* videos = library->videos()) {
        if (auto v = videos->item(entryId)) {
            const bool episode = v->type == video::Type::Episode;
            d.title = episode ? qstr(v->show) : qstr(v->title);
            QStringList parts;
            if (episode) {
                parts << QStringLiteral("S%1 E%2").arg(v->season).arg(v->episode) << qstr(v->title);
            } else if (v->year > 0) {
                parts << QString::number(v->year);
            }
            d.subtitle = parts.join(QStringLiteral(" · "));
            d.duration = v->duration_ms;
            // The same poster as its card in the grid (its movie or show), Plex's or a frame included.
            const video::Filter own = episode
                ? video::Filter{{{video::Field::Type, video::Value{std::int64_t{2}}}, {video::Field::Show, video::Value{v->show}}}}
                : video::Filter{{{video::Field::Type, video::Value{std::int64_t{1}}}, {video::Field::Title, video::Value{v->title}},
                                 {video::Field::Year, video::Value{std::int64_t{v->year}}}}};
            const std::vector<video::Field> by = episode ? std::vector<video::Field>{video::Field::Show}
                                                         : std::vector<video::Field>{video::Field::Title, video::Field::Year};
            if (auto g = videos->groups(by, own, video::GroupOrder::Values, false, std::nullopt, 1); g && !g->items.empty()) {
                d.coverEntry = fields::videoCover(g->items.front());
            } else {
                d.coverEntry = v->poster_entry ? v->poster_entry : (v->has_embedded_poster ? v->entry_id : 0);
            }
            const QVariantMap params = episode ? QVariantMap{{"show", qstr(v->show)}, {"crumb", qstr(v->show)}}
                                               : QVariantMap{{"title", qstr(v->title)}, {"year", v->year}, {"crumb", qstr(v->title)}};
            d.link = {{"categoryId", library->categoryOfFolder(int(v->root_id))},
                      {"page", episode ? QStringLiteral("show") : QStringLiteral("movie")},
                      {"params", params}};
        }
    }
    return d;
}

void PlayerBridge::onStatus(const player::Status& s) {
    const bool entryChanged = s.entry != status_.entry;
    const bool ended = s.state == player::State::Idle && status_.state != player::State::Idle;
    status_ = s;
    position_ = s.position_ms;
    if (entryChanged) {
        now_ = s.entry ? describe(*s.entry) : Description{};
        emit nowPlayingChanged();
    }
    emit statusChanged();
    emit positionChanged();
    // A new item or the end of one: its saved position, viewed mark, and play count moved.
    if (entryChanged || ended) {
        ++revision_;
        emit statesChanged();
    }
}

void PlayerBridge::onQueue() {
    if (!player_) return;
    queue_.clear();
    for (auto id : player_->queue()) queue_ << qint64(id);
    current_ = int(player_->current());
    emit queueChanged();
}

void PlayerBridge::onError(qint64 entryId, const QString& message) {
    const QString name = describe(entryId).title;
    errors_ << (name.isEmpty() ? QStringLiteral("a file") : QStringLiteral("“%1”").arg(name)) + QStringLiteral(" skipped: ") + message;
    errorDelay_.start();
}

void PlayerBridge::flushErrors() {
    if (errors_.isEmpty()) return;
    const QString line = errors_.size() == 1 ? errors_.first()
                                             : QStringLiteral("%1 files skipped (%2)").arg(errors_.size()).arg(errors_.first());
    errors_.clear();
    if (auto* library = LibraryBridge::instance()) emit library->notice(line);
}

void PlayerBridge::playNow(const QVariantList& entryIds, int start) {
    if (player_) player_->play_now(toIds(entryIds), std::size_t(std::max(0, start)));
}
void PlayerBridge::add(const QVariantList& entryIds) {
    if (player_) player_->add(toIds(entryIds));
}
void PlayerBridge::jump(int index) {
    if (player_ && index >= 0) player_->jump(std::size_t(index));
}
void PlayerBridge::remove(int index) {
    if (player_ && index >= 0) player_->remove(std::size_t(index));
}
void PlayerBridge::clear() {
    if (player_) player_->clear();
}
void PlayerBridge::toggle() {
    if (player_) player_->toggle();
}
void PlayerBridge::pause() {
    if (player_) player_->pause();
}
void PlayerBridge::resume() {
    if (player_) player_->resume();
}
void PlayerBridge::stop() {
    if (player_) player_->stop();
}
void PlayerBridge::next() {
    if (player_) player_->next();
}
void PlayerBridge::previous() {
    if (player_) player_->previous();
}
void PlayerBridge::seek(qint64 positionMs) {
    if (player_) player_->seek(positionMs);
}
void PlayerBridge::seekBy(qint64 deltaMs) {
    if (player_) player_->seek_by(deltaMs);
}
void PlayerBridge::setVolume(int volume) {
    if (player_) player_->set_volume(volume);
}
void PlayerBridge::setMuted(bool muted) {
    if (player_) player_->set_muted(muted);
}
void PlayerBridge::selectAudio(int trackId) {
    if (player_) player_->select_audio(trackId);
}
void PlayerBridge::selectSubtitle(int trackId) {
    if (player_) player_->select_subtitle(trackId < 0 ? std::nullopt : std::optional<int>(trackId));
}

bool PlayerBridge::isLiked(qint64 entryId) {
    if (!items_ || entryId <= 0) return false;
    auto s = items_->state(entryId);
    return s && s->liked();
}

void PlayerBridge::setLiked(qint64 entryId, bool liked) {
    if (!items_ || entryId <= 0) return;
    if (auto r = items_->set_liked(entryId, liked, nowNs()); !r) {
        if (auto* library = LibraryBridge::instance()) emit library->notice(QStringLiteral("couldn't save the like: ") + qstr(r.error().message));
        return;
    }
    ++revision_;
    emit statesChanged();
}

QVariantList PlayerBridge::itemStates(const QVariantList& entryIds) {
    QVariantList out;
    if (!items_) return out;
    const auto ids = toIds(entryIds);
    auto states = items_->states(ids);
    if (!states) return out;
    for (const auto& s : *states) {
        out << QVariantMap{{"entryId", qint64(s.entry)}, {"position", qint64(s.position_ms)}, {"duration", qint64(s.duration_ms)},
                           {"viewed", s.viewed}, {"playCount", s.play_count}, {"liked", s.liked()}};
    }
    return out;
}

}
