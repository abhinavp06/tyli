#include "queue_model.hpp"

#include "player_bridge.hpp"

namespace tyli {

QueueModel::QueueModel(QObject* parent) : QAbstractListModel(parent) {
    if (auto* player = PlayerBridge::instance()) {
        connect(player, &PlayerBridge::queueChanged, this, &QueueModel::reload);
        // The playing row's mark moves with the player even when the queue itself doesn't change.
        connect(player, &PlayerBridge::statusChanged, this, [this, player] {
            if (player->current() == current_ || rows_.empty()) return;
            current_ = player->current();
            emit dataChanged(index(0), index(int(rows_.size()) - 1), {PlayingRole, PlayedRole});
            emit countChanged();
        });
    }
    reload();
}

void QueueModel::reload() {
    beginResetModel();
    rows_.clear();
    if (auto* player = PlayerBridge::instance()) {
        for (qint64 id : player->queue()) {
            const auto d = player->describe(id);
            rows_.push_back({id, d.title, d.subtitle, d.duration});
        }
        current_ = player->current();
    }
    endResetModel();
    emit countChanged();
}

qint64 QueueModel::remainingDuration() const {
    qint64 sum = 0;
    for (std::size_t i = std::size_t(std::max(0, current_ + 1)); i < rows_.size(); ++i) sum += rows_[i].duration;
    return sum;
}

int QueueModel::rowCount(const QModelIndex& parent) const { return parent.isValid() ? 0 : int(rows_.size()); }

QVariant QueueModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= int(rows_.size())) return {};
    const Row& r = rows_[std::size_t(index.row())];
    const bool active = PlayerBridge::instance() && PlayerBridge::instance()->active();
    switch (role) {
    case EntryIdRole: return r.entryId;
    case TitleRole: return r.title;
    case SubtitleRole: return r.subtitle;
    case DurationRole: return r.duration;
    case PlayingRole: return active && index.row() == current_;
    case PlayedRole: return index.row() < current_;
    default: return {};
    }
}

QHash<int, QByteArray> QueueModel::roleNames() const {
    return {{EntryIdRole, "entryId"}, {TitleRole, "title"},     {SubtitleRole, "subtitle"},
            {DurationRole, "duration"}, {PlayingRole, "playing"}, {PlayedRole, "played"}};
}

}
