#include "group_model.hpp"

#include "library_bridge.hpp"
#include "media_fields.hpp"
#include "thumbnail_cache.hpp"

#include <QSet>
#include <QTimer>

namespace tyli {

namespace audio = hoardor::audio;
namespace video = hoardor::video;

GroupModel::GroupModel(QObject* parent) : QAbstractListModel(parent) {
    if (auto* bridge = LibraryBridge::instance()) {
        connect(bridge, &LibraryBridge::contentChanged, this, &GroupModel::refresh);
        if (auto* thumbnails = bridge->thumbnails()) {
            connect(thumbnails, &ThumbnailCache::changed, this, [this] {
                if (!rows_.empty()) emit dataChanged(index(0), index(int(rows_.size()) - 1), {ThumbnailRole});
            });
        }
    }
}

void GroupModel::setEngine(const QString& engine) {
    if (engine_ == engine) return;
    engine_ = engine;
    emit queryChanged();
    scheduleReload();
}

void GroupModel::setGroupBy(const QStringList& groupBy) {
    if (groupBy_ == groupBy) return;
    groupBy_ = groupBy;
    emit queryChanged();
    scheduleReload();
}

void GroupModel::setFilter(const QVariantMap& filter) {
    if (filter_ == filter) return;
    filter_ = filter;
    emit queryChanged();
    scheduleReload();
}

void GroupModel::setOrder(const QString& order) {
    if (order_ == order) return;
    order_ = order;
    emit queryChanged();
    scheduleReload();
}

void GroupModel::setDescending(bool descending) {
    if (descending_ == descending) return;
    descending_ = descending;
    emit queryChanged();
    scheduleReload();
}

// Several properties usually change together (a QML binding update): reload once, after them.
void GroupModel::scheduleReload() {
    if (reloadPending_) return;
    reloadPending_ = true;
    QTimer::singleShot(0, this, [this] {
        reloadPending_ = false;
        reload();
    });
}

void GroupModel::reload() {
    // The total first: resetting lets the view fetch its first page at once (endResetModel),
    // which may also reach the end, so nothing after the reset may depend on that state.
    const int total = groupBy_.isEmpty() ? 0 : countGroups();
    beginResetModel();
    rows_.clear();
    next_.reset();
    atEnd_ = groupBy_.isEmpty();
    total_ = total;
    error_.clear();
    endResetModel();
    if (rows_.empty() && !atEnd_) fetchMore({});
    emit countChanged();
}

void GroupModel::refresh() {
    if (reloadPending_ || groupBy_.isEmpty()) return;  // a full reload is coming anyway
    if (rows_.empty()) {
        reload();
        return;
    }
    // The same range again, from the start.
    std::vector<Row> fresh;
    std::optional<hoardor::core::Cursor> cursor;
    const std::size_t wanted = std::max<std::size_t>(rows_.size(), PageSize);
    QString error;
    while (fresh.size() < wanted) {
        auto rows = page(cursor, error);
        if (!rows) return;  // keep what's shown; the next refresh tries again
        for (auto& r : *rows) fresh.push_back(std::move(r));
        if (!cursor) break;
    }
    total_ = countGroups();

    // Old rows that are gone.
    QSet<QString> freshKeys;
    for (const Row& r : fresh) freshKeys.insert(keyOf(r));
    for (int i = int(rows_.size()) - 1; i >= 0; --i) {
        if (freshKeys.contains(keyOf(rows_[std::size_t(i)]))) continue;
        beginRemoveRows({}, i, i);
        rows_.erase(rows_.begin() + i);
        endRemoveRows();
    }
    // Walk the fresh order: keep, update, move, or insert.
    for (std::size_t i = 0; i < fresh.size(); ++i) {
        const QString key = keyOf(fresh[i]);
        if (i < rows_.size() && keyOf(rows_[i]) == key) {
            rows_[i] = fresh[i];
            emit dataChanged(index(int(i)), index(int(i)));
            continue;
        }
        std::size_t found = i + 1;
        while (found < rows_.size() && keyOf(rows_[found]) != key) ++found;
        if (found < rows_.size()) {
            beginMoveRows({}, int(found), int(found), {}, int(i));
            Row moved = std::move(rows_[found]);
            rows_.erase(rows_.begin() + std::ptrdiff_t(found));
            rows_.insert(rows_.begin() + std::ptrdiff_t(i), std::move(moved));
            endMoveRows();
            rows_[i] = fresh[i];
            emit dataChanged(index(int(i)), index(int(i)));
        } else {
            beginInsertRows({}, int(i), int(i));
            rows_.insert(rows_.begin() + std::ptrdiff_t(i), fresh[i]);
            endInsertRows();
        }
    }
    // Rows past the fresh range (fewer groups than before).
    if (rows_.size() > fresh.size()) {
        beginRemoveRows({}, int(fresh.size()), int(rows_.size()) - 1);
        rows_.resize(fresh.size());
        endRemoveRows();
    }
    next_ = cursor;
    atEnd_ = !next_.has_value();
    emit countChanged();
}

int GroupModel::countGroups() const {
    auto* bridge = LibraryBridge::instance();
    if (!bridge || !bridge->ready()) return 0;
    if (engine_ == QLatin1String("video")) {
        std::vector<video::Field> by;
        for (const auto& name : groupBy_) {
            if (auto f = fields::video(name)) by.push_back(*f);
        }
        const auto filter = fields::videoFilter(filter_);
        if (!filter || by.size() != std::size_t(groupBy_.size())) return 0;
        auto n = bridge->videos()->group_count(by, *filter);
        return n ? int(*n) : 0;
    }
    std::vector<audio::Field> by;
    for (const auto& name : groupBy_) {
        if (auto f = fields::audio(name)) by.push_back(*f);
    }
    const auto filter = fields::audioFilter(filter_);
    if (!filter || by.size() != std::size_t(groupBy_.size())) return 0;
    auto n = bridge->tracks()->group_count(by, *filter);
    return n ? int(*n) : 0;
}

namespace {

template <class Order>
Order groupOrder(const QString& name) {
    if (name == QLatin1String("added")) return Order::AddedLast;
    if (name == QLatin1String("year")) return Order::Year;
    if (name == QLatin1String("count")) {
        if constexpr (std::is_same_v<Order, audio::GroupOrder>) return Order::Tracks;
        else return Order::Items;
    }
    return Order::Values;
}

}

std::optional<std::vector<GroupModel::Row>> GroupModel::page(std::optional<hoardor::core::Cursor>& cursor, QString& error) {
    auto* bridge = LibraryBridge::instance();
    if (!bridge || !bridge->ready()) return std::vector<Row>{};
    std::vector<Row> out;
    if (engine_ == QLatin1String("video")) {
        std::vector<video::Field> by;
        for (const auto& name : groupBy_) {
            auto f = fields::video(name);
            if (!f) {
                error = QStringLiteral("unknown field %1").arg(name);
                return std::nullopt;
            }
            by.push_back(*f);
        }
        const auto filter = fields::videoFilter(filter_);
        if (!filter) {
                error = QStringLiteral("unknown filter field");
                return std::nullopt;
            }
        auto result = bridge->videos()->groups(by, *filter, groupOrder<video::GroupOrder>(order_), descending_, cursor, PageSize);
        if (!result) {
                error = QString::fromStdString(result.error().message);
                return std::nullopt;
            }
        for (const auto& g : result->items) {
            Row r;
            for (const auto& v : g.values) r.values << QString::fromStdString(v);
            r.count = qint64(g.items);
            r.durationMs = g.duration_ms;
            r.addedLastMs = g.added_last_ns / 1'000'000;
            r.yearMin = g.year_min;
            r.yearMax = g.year_max;
            r.maxHeight = g.max_height;
            r.hdr = g.any_hdr;
            r.online = g.any_online;
            r.coverEntry = fields::videoCover(g);
            out.push_back(std::move(r));
        }
        cursor = result->next;
    } else {
        std::vector<audio::Field> by;
        for (const auto& name : groupBy_) {
            auto f = fields::audio(name);
            if (!f) {
                error = QStringLiteral("unknown field %1").arg(name);
                return std::nullopt;
            }
            by.push_back(*f);
        }
        const auto filter = fields::audioFilter(filter_);
        if (!filter) {
                error = QStringLiteral("unknown filter field");
                return std::nullopt;
            }
        auto result = bridge->tracks()->groups(by, *filter, groupOrder<audio::GroupOrder>(order_), descending_, cursor, PageSize);
        if (!result) {
                error = QString::fromStdString(result.error().message);
                return std::nullopt;
            }
        for (const auto& g : result->items) {
            Row r;
            for (const auto& v : g.values) r.values << QString::fromStdString(v);
            r.count = qint64(g.tracks);
            r.durationMs = g.duration_ms;
            r.addedLastMs = g.added_last_ns / 1'000'000;
            r.yearMin = g.year_min;
            r.yearMax = g.year_max;
            r.online = g.any_online;
            r.coverEntry = g.cover_entry;
            out.push_back(std::move(r));
        }
        cursor = result->next;
    }
    return out;
}

int GroupModel::rowCount(const QModelIndex& parent) const { return parent.isValid() ? 0 : int(rows_.size()); }

bool GroupModel::canFetchMore(const QModelIndex& parent) const { return !parent.isValid() && !atEnd_; }

void GroupModel::fetchMore(const QModelIndex& parent) {
    if (parent.isValid() || atEnd_) return;
    QString error;
    auto rows = page(next_, error);
    if (!rows) {
        atEnd_ = true;
        error_ = error;
        emit countChanged();
        return;
    }
    atEnd_ = !next_.has_value();
    if (rows->empty()) return;
    beginInsertRows({}, int(rows_.size()), int(rows_.size() + rows->size()) - 1);
    for (auto& r : *rows) rows_.push_back(std::move(r));
    endInsertRows();
    emit countChanged();
}

QVariant GroupModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= int(rows_.size())) return {};
    const Row& r = rows_[std::size_t(index.row())];
    switch (role) {
    case ValuesRole: return r.values;
    case CountRole: return r.count;
    case DurationRole: return r.durationMs;
    case AddedLastRole: return r.addedLastMs;
    case YearMinRole: return r.yearMin;
    case YearMaxRole: return r.yearMax;
    case OnlineRole: return r.online;
    case CoverEntryRole: return r.coverEntry;
    case MaxHeightRole: return r.maxHeight;
    case HdrRole: return r.hdr;
    case ThumbnailRole: {
        auto* bridge = LibraryBridge::instance();
        return bridge && bridge->thumbnails() ? bridge->thumbnails()->url(r.coverEntry) : QString();
    }
    default: return {};
    }
}

QHash<int, QByteArray> GroupModel::roleNames() const {
    return {
        {ValuesRole, "values"},       {CountRole, "count"},       {DurationRole, "duration"},
        {AddedLastRole, "addedLast"}, {YearMinRole, "yearMin"},   {YearMaxRole, "yearMax"},
        {OnlineRole, "online"},       {CoverEntryRole, "coverEntry"}, {MaxHeightRole, "maxHeight"},
        {HdrRole, "hdr"},             {ThumbnailRole, "thumbnail"},
    };
}

QVariantMap GroupModel::get(int row) const {
    QVariantMap out;
    if (row < 0 || row >= rowCount()) return out;
    const auto names = roleNames();
    for (auto it = names.cbegin(); it != names.cend(); ++it) out[QString::fromUtf8(it.value())] = data(index(row), it.key());
    return out;
}

}
