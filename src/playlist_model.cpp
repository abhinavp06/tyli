#include "playlist_model.hpp"

#include "library_bridge.hpp"
#include "player_bridge.hpp"

#include <hoardor/audio/audio.hpp>
#include <hoardor/player/player.hpp>

#include <QSet>

namespace tyli {

PlaylistModel::PlaylistModel(QObject* parent) : QAbstractListModel(parent) {
    if (auto* player = PlayerBridge::instance()) {
        connect(player, &PlayerBridge::playlistsChanged, this, &PlaylistModel::reload);
        // Liked Songs follows the hearts; a playlist's rows show them too.
        connect(player, &PlayerBridge::statesChanged, this, [this] {
            if (playlistId_ == -1) reload();
            else if (!rows_.empty()) {
                for (auto& r : rows_) r.liked = PlayerBridge::instance()->isLiked(r.entryId);
                emit dataChanged(index(0), index(int(rows_.size()) - 1), {LikedRole});
            }
        });
    }
}

void PlaylistModel::setPlaylistId(qint64 id) {
    if (id == playlistId_) return;
    playlistId_ = id;
    emit playlistIdChanged();
    reload();
}

void PlaylistModel::reload() {
    beginResetModel();
    rows_.clear();
    next_.reset();
    done_ = playlistId_ == 0;
    endResetModel();
    if (!done_) fetchMore({});
    emit countChanged();
}

void PlaylistModel::fill(Row& row) const {
    auto* library = LibraryBridge::instance();
    if (!library || !library->tracks()) return;
    if (auto t = library->tracks()->track(row.entryId)) {
        row.title = QString::fromStdString(t->title);
        QStringList artists;
        for (const auto& a : t->artists) artists << QString::fromStdString(a);
        row.albumArtist = QString::fromStdString(t->album_artist);
        row.artist = artists.isEmpty() ? row.albumArtist : artists.join(QStringLiteral(", "));
        row.album = QString::fromStdString(t->album);
        row.duration = t->duration_ms;
        row.online = t->root_online;
        row.rootId = t->root_id;
    }
    if (auto* player = PlayerBridge::instance()) row.liked = playlistId_ == -1 || player->isLiked(row.entryId);
}

bool PlaylistModel::canFetchMore(const QModelIndex& parent) const { return !parent.isValid() && !done_; }

void PlaylistModel::fetchMore(const QModelIndex& parent) {
    if (parent.isValid() || done_) return;
    auto* player = PlayerBridge::instance();
    auto* items = player ? player->items() : nullptr;
    if (!items) {
        done_ = true;
        return;
    }
    std::vector<Row> page;
    std::optional<hoardor::core::Cursor> next;
    const auto row = [](qint64 itemId, qint64 entryId, std::int64_t addedNs) {
        Row r;
        r.itemId = itemId;
        r.entryId = entryId;
        r.added = addedNs / 1'000'000;
        return r;
    };
    if (playlistId_ == -1) {
        auto liked = items->liked(next_, PageSize);
        if (!liked) {
            done_ = true;
            return;
        }
        for (const auto& l : liked->items) page.push_back(row(l.entry, l.entry, l.liked_ns));
        next = liked->next;
    } else {
        auto rows = items->playlist_items(playlistId_, next_, PageSize);
        if (!rows) {
            done_ = true;
            return;
        }
        for (const auto& r : rows->items) page.push_back(row(r.id, r.entry, r.added_ns));
        next = rows->next;
    }
    for (auto& r : page) fill(r);
    next_ = next;
    done_ = !next;
    if (page.empty()) return;
    beginInsertRows({}, int(rows_.size()), int(rows_.size() + page.size()) - 1);
    for (auto& r : page) rows_.push_back(std::move(r));
    endInsertRows();
    emit countChanged();
}

QVariantList PlaylistModel::entryIds() const {
    // The whole list (not only the loaded pages): what plays is every row.
    QVariantList out;
    if (auto* player = PlayerBridge::instance()) {
        out = playlistId_ == -1 ? player->likedEntries() : player->playlistEntries(playlistId_);
    }
    return out;
}

QVariantList PlaylistModel::coverEntries(int max) const {
    namespace a = hoardor::audio;
    QVariantList out;
    auto* library = LibraryBridge::instance();
    if (!library || !library->tracks()) return out;
    QSet<QString> seen;
    const std::vector<a::Field> by{a::Field::AlbumArtist, a::Field::Album};
    for (const Row& r : rows_) {
        if (out.size() >= max) break;
        const QString key = r.albumArtist.toCaseFolded() + QChar(0x1f) + r.album.toCaseFolded();
        if (r.album.isEmpty() || seen.contains(key)) continue;
        seen.insert(key);
        const a::Filter one{{{a::Field::AlbumArtist, a::Value{r.albumArtist.toStdString()}}, {a::Field::Album, a::Value{r.album.toStdString()}}}};
        if (auto g = library->tracks()->groups(by, one, a::GroupOrder::Values, false, std::nullopt, 1); g && !g->items.empty()) {
            out << qint64(g->items.front().cover_entry);
        }
    }
    return out;
}

int PlaylistModel::rowCount(const QModelIndex& parent) const { return parent.isValid() ? 0 : int(rows_.size()); }

QVariant PlaylistModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= int(rows_.size())) return {};
    const Row& r = rows_[std::size_t(index.row())];
    switch (role) {
    case ItemIdRole: return r.itemId;
    case EntryIdRole: return r.entryId;
    case TitleRole: return r.title;
    case ArtistRole: return r.artist;
    case AlbumRole: return r.album;
    case AlbumArtistRole: return r.albumArtist;
    case DurationRole: return r.duration;
    case OnlineRole: return r.online;
    case LikedRole: return r.liked;
    case AddedRole: return r.added;
    case RootIdRole: return r.rootId;
    default: return {};
    }
}

QHash<int, QByteArray> PlaylistModel::roleNames() const {
    return {{ItemIdRole, "itemId"},   {EntryIdRole, "entryId"},   {TitleRole, "title"},   {ArtistRole, "artist"},
            {AlbumRole, "album"},     {AlbumArtistRole, "albumArtist"}, {DurationRole, "duration"}, {OnlineRole, "online"},
            {LikedRole, "liked"},     {AddedRole, "added"},       {RootIdRole, "rootId"}};
}

}
