#include "item_model.hpp"

#include "library_bridge.hpp"
#include "media_fields.hpp"

#include <QTimer>

namespace tyli {

namespace audio = hoardor::audio;
namespace video = hoardor::video;

namespace {

QStringList strings(const std::vector<std::string>& values) {
    QStringList out;
    for (const auto& v : values) out << QString::fromStdString(v);
    return out;
}

QVariantList streams(const std::vector<video::Stream>& list) {
    QVariantList out;
    for (const auto& s : list) {
        out << QVariantMap{{"language", QString::fromStdString(s.language)}, {"codec", QString::fromStdString(s.codec)},
                           {"channels", s.channels}, {"title", QString::fromStdString(s.title)}};
    }
    return out;
}

// The role names, in one list so roleNames() and the maps agree.
const QList<QByteArray>& names() {
    static const QList<QByteArray> n = {
        "entryId", "title", "album", "albumArtist", "artists", "genres", "track", "trackTotal", "disc", "discTotal",
        "year", "date", "duration", "bitrate", "sampleRate", "bitDepth", "channels", "codec", "lossless",
        "hasEmbeddedCover", "fromName", "rootId", "online", "addedMs", "size",
        "type", "show", "season", "episode", "plot", "directors", "width", "height", "hdr", "videoCodec",
        "frameRate", "audioStreams", "subtitleStreams", "posterEntry", "source",
    };
    return n;
}

}

QVariantMap trackToMap(const audio::Track& t) {
    return {
        {"entryId", qint64(t.entry_id)}, {"title", QString::fromStdString(t.title)}, {"album", QString::fromStdString(t.album)},
        {"albumArtist", QString::fromStdString(t.album_artist)}, {"artists", strings(t.artists)}, {"genres", strings(t.genres)},
        {"track", t.track}, {"trackTotal", t.track_total}, {"disc", t.disc}, {"discTotal", t.disc_total}, {"year", t.year},
        {"date", QString::fromStdString(t.date)}, {"duration", qint64(t.duration_ms)}, {"bitrate", t.bitrate_kbps},
        {"sampleRate", t.sample_rate}, {"bitDepth", t.bit_depth}, {"channels", t.channels},
        {"codec", QString::fromStdString(t.codec)}, {"lossless", t.lossless}, {"hasEmbeddedCover", t.has_embedded_cover},
        {"fromName", t.title_from_name}, {"rootId", qint64(t.root_id)}, {"online", t.root_online},
        {"addedMs", qint64(t.added_ns / 1'000'000)}, {"size", qint64(t.size)},
    };
}

QVariantMap itemToMap(const video::Item& i) {
    return {
        {"entryId", qint64(i.entry_id)}, {"type", i.type == video::Type::Episode ? QStringLiteral("episode") : QStringLiteral("movie")},
        {"title", QString::fromStdString(i.title)}, {"show", QString::fromStdString(i.show)}, {"season", i.season},
        {"episode", i.episode}, {"year", i.year}, {"date", QString::fromStdString(i.date)}, {"plot", QString::fromStdString(i.plot)},
        {"genres", strings(i.genres)}, {"directors", strings(i.directors)}, {"duration", qint64(i.duration_ms)},
        {"width", i.width}, {"height", i.height}, {"hdr", QString::fromStdString(i.hdr)},
        {"videoCodec", QString::fromStdString(i.video_codec)}, {"frameRate", i.frame_rate_milli / 1000.0},
        {"audioStreams", streams(i.audio)}, {"subtitleStreams", streams(i.subtitles)}, {"fromName", i.from_name},
        {"posterEntry", qint64(i.poster_entry ? i.poster_entry : (i.has_embedded_poster ? i.entry_id : 0))},
        {"source", i.source == video::Source::Nfo ? QStringLiteral("nfo") : i.source == video::Source::Tags ? QStringLiteral("tags") : QStringLiteral("name")},
        {"rootId", qint64(i.root_id)}, {"online", i.root_online}, {"addedMs", qint64(i.added_ns / 1'000'000)},
        {"size", qint64(i.size)},
    };
}

ItemModel::ItemModel(QObject* parent) : QAbstractListModel(parent) {
    if (auto* bridge = LibraryBridge::instance()) connect(bridge, &LibraryBridge::contentChanged, this, &ItemModel::scheduleReload);
}

void ItemModel::setEngine(const QString& engine) {
    if (engine_ == engine) return;
    engine_ = engine;
    emit queryChanged();
    scheduleReload();
}

void ItemModel::setFilter(const QVariantMap& filter) {
    if (filter_ == filter) return;
    filter_ = filter;
    emit queryChanged();
    scheduleReload();
}

void ItemModel::setOrder(const QStringList& order) {
    if (order_ == order) return;
    order_ = order;
    emit queryChanged();
    scheduleReload();
}

void ItemModel::scheduleReload() {
    if (reloadPending_) return;
    reloadPending_ = true;
    QTimer::singleShot(0, this, [this] {
        reloadPending_ = false;
        reload();
    });
}

void ItemModel::reload() {
    beginResetModel();
    rows_.clear();
    next_.reset();
    atEnd_ = false;
    error_.clear();
    endResetModel();
    if (rows_.empty() && !atEnd_) fetchMore({});  // the view may have fetched already (endResetModel)
    emit countChanged();
}

int ItemModel::rowCount(const QModelIndex& parent) const { return parent.isValid() ? 0 : int(rows_.size()); }

bool ItemModel::canFetchMore(const QModelIndex& parent) const { return !parent.isValid() && !atEnd_; }

void ItemModel::fetchMore(const QModelIndex& parent) {
    if (parent.isValid() || atEnd_) return;
    auto* bridge = LibraryBridge::instance();
    if (!bridge || !bridge->ready()) {
        atEnd_ = true;
        return;
    }
    std::vector<QVariantMap> page;
    const auto fail = [this](QString message) {
        atEnd_ = true;
        error_ = std::move(message);
        emit countChanged();
    };
    if (engine_ == QLatin1String("video")) {
        const auto filter = fields::videoFilter(filter_);
        const auto order = fields::videoOrder(order_);
        if (!filter || !order) return fail(QStringLiteral("unknown field"));
        auto result = bridge->videos()->items(*filter, *order, next_, PageSize);
        if (!result) return fail(QString::fromStdString(result.error().message));
        for (const auto& i : result->items) page.push_back(itemToMap(i));
        next_ = result->next;
    } else {
        const auto filter = fields::audioFilter(filter_);
        const auto order = fields::audioOrder(order_);
        if (!filter || !order) return fail(QStringLiteral("unknown field"));
        auto result = bridge->tracks()->tracks(*filter, *order, next_, PageSize);
        if (!result) return fail(QString::fromStdString(result.error().message));
        for (const auto& t : result->items) page.push_back(trackToMap(t));
        next_ = result->next;
    }
    atEnd_ = !next_.has_value();
    if (page.empty()) return;
    beginInsertRows({}, int(rows_.size()), int(rows_.size() + page.size()) - 1);
    for (auto& r : page) rows_.push_back(std::move(r));
    endInsertRows();
    emit countChanged();
}

QVariant ItemModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= int(rows_.size())) return {};
    const int i = role - (Qt::UserRole + 1);
    if (i < 0 || i >= names().size()) return {};
    return rows_[std::size_t(index.row())].value(QString::fromLatin1(names()[i]));
}

QHash<int, QByteArray> ItemModel::roleNames() const {
    QHash<int, QByteArray> out;
    for (int i = 0; i < names().size(); ++i) out[Qt::UserRole + 1 + i] = names()[i];
    return out;
}

QVariantMap ItemModel::get(int row) const { return row >= 0 && row < rowCount() ? rows_[std::size_t(row)] : QVariantMap{}; }

}
