#pragma once

#include <QAbstractListModel>
#include <QString>
#include <QtQml/qqmlregistration.h>

#include <cstdint>
#include <vector>

namespace tyli {

// The files of one folder, read from hoardor's database a page at a time as the view
// scrolls (canFetchMore / fetchMore), so a folder of 500k files never loads at once.
// Reads only SQLite: an offline folder's files are listed without touching its drive.
class FolderFilesModel : public QAbstractListModel {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(int folderId READ folderId WRITE setFolderId NOTIFY folderIdChanged)
    Q_PROPERTY(int count READ count NOTIFY countChanged)  // rows loaded so far, not the folder's total

public:
    enum Role { RelativePathRole = Qt::UserRole + 1, SizeRole, KindRole, UnsettledRole, MtimeRole };
    static constexpr int PageSize = 200;

    explicit FolderFilesModel(QObject* parent = nullptr);

    int folderId() const { return folderId_; }
    void setFolderId(int folderId);
    int count() const { return int(rows_.size()); }

    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;
    bool canFetchMore(const QModelIndex& parent) const override;
    void fetchMore(const QModelIndex& parent) override;

    // Drops what's loaded and starts again from the first page (after a sync, for example).
    Q_INVOKABLE void reload();

signals:
    void folderIdChanged();
    void countChanged();

private:
    struct Row {
        std::int64_t id;
        QString relativePath;
        qint64 size;
        QString kind;
        bool unsettled;
        qint64 mtimeMs;
    };

    int folderId_ = -1;
    std::vector<Row> rows_;
    bool atEnd_ = true;
};

}
