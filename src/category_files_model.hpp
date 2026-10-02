#pragma once

#include <QAbstractListModel>
#include <QString>
#include <QtQml/qqmlregistration.h>

#include <cstdint>
#include <vector>

namespace tyli {

// The files of one category, across all its folders (one folder after another, each
// in database order), read a page at a time as the view scrolls (canFetchMore /
// fetchMore). A category of 500k files never loads at once. Reads only SQLite, so an
// offline folder's files are listed without touching its drive.
class CategoryFilesModel : public QAbstractListModel {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(int categoryId READ categoryId WRITE setCategoryId NOTIFY categoryIdChanged)
    Q_PROPERTY(int count READ count NOTIFY countChanged)  // rows loaded so far, not the category's total

public:
    enum Role {
        RelativePathRole = Qt::UserRole + 1,
        FolderNameRole,
        FolderOnlineRole,
        SizeRole,
        KindRole,
        UnsettledRole,
        MtimeRole,
    };
    static constexpr int PageSize = 200;

    explicit CategoryFilesModel(QObject* parent = nullptr);

    int categoryId() const { return categoryId_; }
    void setCategoryId(int categoryId);
    int count() const { return int(rows_.size()); }

    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;
    bool canFetchMore(const QModelIndex& parent) const override;
    void fetchMore(const QModelIndex& parent) override;

    // Re-reads the category's folders and starts again from the first page. Runs by
    // itself whenever the library's folders change (a sync, a folder added or moved).
    Q_INVOKABLE void reload();

signals:
    void categoryIdChanged();
    void countChanged();

private:
    struct Folder {
        std::int64_t id;
        QString name;
        bool online;
    };
    struct Row {
        std::int64_t id;
        QString relativePath;
        qint64 size;
        QString kind;
        qint64 mtimeMs;
        int folder;  // index into folders_
        bool unsettled;
    };

    int categoryId_ = -1;
    std::vector<Folder> folders_;
    std::vector<Row> rows_;
    std::size_t folderIndex_ = 0;  // the folder being paged
    std::int64_t after_ = 0;       // the last entry id read from it
    bool atEnd_ = true;
};

}
