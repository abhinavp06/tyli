#pragma once

#include <hoardor/core/page.hpp>

#include <QAbstractListModel>
#include <QStringList>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>

#include <optional>

namespace tyli {

// Groups of tracks or videos from hoardor's generic `groups` query: albums
// (groupBy ["albumArtist", "album"]), artists, genres, movies (["title", "year"]), shows,
// seasons. Pages from SQLite as the view scrolls; never touches a drive. Reloads when any
// property changes and whenever the library's content changes (a sync, a metadata pass).
class GroupModel : public QAbstractListModel {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString engine READ engine WRITE setEngine NOTIFY queryChanged)          // "audio" | "video"
    Q_PROPERTY(QStringList groupBy READ groupBy WRITE setGroupBy NOTIFY queryChanged)
    Q_PROPERTY(QVariantMap filter READ filter WRITE setFilter NOTIFY queryChanged)      // {field: value}
    Q_PROPERTY(QString order READ order WRITE setOrder NOTIFY queryChanged)             // "values" | "added" | "year" | "count"
    Q_PROPERTY(bool descending READ descending WRITE setDescending NOTIFY queryChanged)
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)                              // loaded so far
    Q_PROPERTY(int total READ total NOTIFY countChanged)                                 // every group
    Q_PROPERTY(QString error READ error NOTIFY countChanged)

public:
    enum Role {
        ValuesRole = Qt::UserRole + 1,  // QStringList: the group-by fields' display values
        CountRole,                      // tracks or items
        DurationRole,                   // ms
        AddedLastRole,                  // ms since the epoch
        YearMinRole,
        YearMaxRole,
        OnlineRole,
        CoverEntryRole,                 // the entry whose art stands for the group (0: none)
        MaxHeightRole,                  // video
        HdrRole,                        // video
        ThumbnailRole,                  // a file: URL of the cached thumbnail, or ""
    };
    static constexpr int PageSize = 120;

    explicit GroupModel(QObject* parent = nullptr);

    QString engine() const { return engine_; }
    void setEngine(const QString& engine);
    QStringList groupBy() const { return groupBy_; }
    void setGroupBy(const QStringList& groupBy);
    QVariantMap filter() const { return filter_; }
    void setFilter(const QVariantMap& filter);
    QString order() const { return order_; }
    void setOrder(const QString& order);
    bool descending() const { return descending_; }
    void setDescending(bool descending);
    int total() const { return total_; }
    QString error() const { return error_; }

    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;
    bool canFetchMore(const QModelIndex& parent) const override;
    void fetchMore(const QModelIndex& parent) override;

    // The row's value for a role by name, for QML code outside delegates.
    Q_INVOKABLE QVariantMap get(int row) const;

signals:
    void queryChanged();
    void countChanged();

private:
    struct Row {
        QStringList values;
        qint64 count = 0, durationMs = 0, addedLastMs = 0;
        int yearMin = 0, yearMax = 0, maxHeight = 0;
        bool online = false, hdr = false;
        qint64 coverEntry = 0;
    };
    void scheduleReload();
    void reload();
    int countGroups() const;
    std::optional<std::vector<Row>> page(std::optional<hoardor::core::Cursor>& cursor, QString& error);
    // New content (a sync, tags read): re-read the loaded range and apply the differences
    // (insert, update, move, remove), so the view keeps its place instead of resetting.
    void refresh();
    static QString keyOf(const Row& row) { return row.values.join(QChar(0x1F)); }

    QString engine_ = QStringLiteral("audio");
    QStringList groupBy_;
    QVariantMap filter_;
    QString order_ = QStringLiteral("values");
    bool descending_ = false;

    std::vector<Row> rows_;
    std::optional<hoardor::core::Cursor> next_;
    bool atEnd_ = true;
    int total_ = 0;
    QString error_;
    bool reloadPending_ = false;
};

}
