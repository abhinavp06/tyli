#pragma once

#include <hoardor/audio/audio.hpp>
#include <hoardor/core/page.hpp>
#include <hoardor/video/video.hpp>

#include <QAbstractListModel>
#include <QStringList>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>

#include <optional>

namespace tyli {

// Tracks or videos from hoardor's generic `tracks` / `items` query, paged from SQLite.
// Roles cover both engines; the other engine's roles are empty.
class ItemModel : public QAbstractListModel {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString engine READ engine WRITE setEngine NOTIFY queryChanged)      // "audio" | "video"
    Q_PROPERTY(QVariantMap filter READ filter WRITE setFilter NOTIFY queryChanged)  // {field: value}
    Q_PROPERTY(QStringList order READ order WRITE setOrder NOTIFY queryChanged)     // ["disc", "track"], "-height"
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
    Q_PROPERTY(QString error READ error NOTIFY countChanged)

public:
    static constexpr int PageSize = 200;
    explicit ItemModel(QObject* parent = nullptr);

    QString engine() const { return engine_; }
    void setEngine(const QString& engine);
    QVariantMap filter() const { return filter_; }
    void setFilter(const QVariantMap& filter);
    QStringList order() const { return order_; }
    void setOrder(const QStringList& order);
    QString error() const { return error_; }

    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;
    bool canFetchMore(const QModelIndex& parent) const override;
    void fetchMore(const QModelIndex& parent) override;
    Q_INVOKABLE QVariantMap get(int row) const;

signals:
    void queryChanged();
    void countChanged();

private:
    void scheduleReload();
    void reload();

    QString engine_ = QStringLiteral("audio");
    QVariantMap filter_;
    QStringList order_;
    std::vector<QVariantMap> rows_;  // one map per row: a few hundred at most on screen
    std::optional<hoardor::core::Cursor> next_;
    bool atEnd_ = true;
    bool reloadPending_ = false;
    QString error_;
};

// The QML map of one track / video (shared with the bridge's copy lists).
QVariantMap trackToMap(const hoardor::audio::Track& track);
QVariantMap itemToMap(const hoardor::video::Item& item);

}
