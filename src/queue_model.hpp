#pragma once

#include <QAbstractListModel>
#include <QtQml/qqmlregistration.h>

namespace tyli {

// The player's queue for the queue page (features/player_ui.md §1.3): one row per entry,
// named from the library on the UI thread. A queue is short, so a change resets the model.
class QueueModel : public QAbstractListModel {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    // The length of everything after the current item.
    Q_PROPERTY(qint64 remainingDuration READ remainingDuration NOTIFY countChanged)

public:
    enum Role { EntryIdRole = Qt::UserRole + 1, TitleRole, SubtitleRole, DurationRole, PlayingRole, PlayedRole };

    explicit QueueModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;
    int count() const { return int(rows_.size()); }
    qint64 remainingDuration() const;

signals:
    void countChanged();

private:
    struct Row {
        qint64 entryId = 0;
        QString title, subtitle;
        qint64 duration = 0;
    };
    void reload();

    std::vector<Row> rows_;
    int current_ = 0;
};

}
