#pragma once

#include <QAbstractListModel>
#include <QtQml/qqmlregistration.h>

#include <hoardor/core/page.hpp>

#include <optional>
#include <vector>

namespace tyli {

// The rows of one playlist, or of Liked Songs (playlistId -1), for their pages
// (features/playlists_ui.md §1.2–1.3). Pages come from hoardor's player::Library as the view
// scrolls; each row's track is looked up in the audio engine (the UI connection). It reloads
// when a playlist changes (Player.playlistsChanged) and, for Liked Songs, when likes do.
class PlaylistModel : public QAbstractListModel {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(qint64 playlistId READ playlistId WRITE setPlaylistId NOTIFY playlistIdChanged)
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    enum Role {
        ItemIdRole = Qt::UserRole + 1, EntryIdRole, TitleRole, ArtistRole, AlbumRole, AlbumArtistRole,
        DurationRole, OnlineRole, LikedRole, AddedRole, RootIdRole
    };
    static constexpr int PageSize = 200;

    explicit PlaylistModel(QObject* parent = nullptr);

    qint64 playlistId() const { return playlistId_; }
    void setPlaylistId(qint64 id);
    int count() const { return int(rows_.size()); }

    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;
    bool canFetchMore(const QModelIndex& parent) const override;
    void fetchMore(const QModelIndex& parent) override;

    // Every row's entry id in order (to play from a row), and a row's item id.
    Q_INVOKABLE QVariantList entryIds() const;
    Q_INVOKABLE void reload();

signals:
    void playlistIdChanged();
    void countChanged();

private:
    struct Row {
        qint64 itemId = 0, entryId = 0;
        QString title, artist, album, albumArtist;
        qint64 duration = 0, added = 0, rootId = 0;
        bool online = true, liked = false;
    };
    void fill(Row& row) const;

    qint64 playlistId_ = 0;   // 0: nothing; -1: Liked Songs
    std::vector<Row> rows_;
    std::optional<hoardor::core::Cursor> next_;
    bool done_ = true;
};

}
