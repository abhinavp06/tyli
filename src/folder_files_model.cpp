#include "folder_files_model.hpp"

#include "library_bridge.hpp"

namespace tyli {

FolderFilesModel::FolderFilesModel(QObject* parent) : QAbstractListModel(parent) {}

void FolderFilesModel::setFolderId(int folderId) {
    if (folderId_ == folderId) return;
    folderId_ = folderId;
    emit folderIdChanged();
    reload();
}

void FolderFilesModel::reload() {
    beginResetModel();
    rows_.clear();
    rows_.shrink_to_fit();
    atEnd_ = folderId_ < 0;
    endResetModel();
    emit countChanged();
}

int FolderFilesModel::rowCount(const QModelIndex& parent) const { return parent.isValid() ? 0 : int(rows_.size()); }

QVariant FolderFilesModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= int(rows_.size())) return {};
    const Row& row = rows_[std::size_t(index.row())];
    switch (role) {
    case RelativePathRole:
    case Qt::DisplayRole: return row.relativePath;
    case SizeRole: return row.size;
    case KindRole: return row.kind;
    case UnsettledRole: return row.unsettled;
    case MtimeRole: return row.mtimeMs;
    default: return {};
    }
}

QHash<int, QByteArray> FolderFilesModel::roleNames() const {
    return {{RelativePathRole, "relativePath"},
            {SizeRole, "size"},
            {KindRole, "kind"},
            {UnsettledRole, "unsettled"},
            {MtimeRole, "mtime"}};
}

bool FolderFilesModel::canFetchMore(const QModelIndex& parent) const { return !parent.isValid() && !atEnd_; }

void FolderFilesModel::fetchMore(const QModelIndex& parent) {
    if (parent.isValid() || atEnd_) return;
    LibraryBridge* bridge = LibraryBridge::instance();
    hoardor::file::Library* library = bridge ? bridge->library() : nullptr;
    if (!library) {
        atEnd_ = true;
        return;
    }
    // Keyset paging (after the last id seen), so every page costs the same however deep you scroll.
    const std::int64_t after = rows_.empty() ? 0 : rows_.back().id;
    auto page = library->entries(folderId_, after, PageSize);
    if (!page) {
        atEnd_ = true;
        emit bridge->errorOccurred(QString::fromStdString(page.error().message));
        return;
    }
    if (page->size() < std::size_t(PageSize)) atEnd_ = true;
    if (page->empty()) return;

    const int first = int(rows_.size());
    beginInsertRows({}, first, first + int(page->size()) - 1);
    for (const auto& e : *page) {
        rows_.push_back(Row{e.id, QString::fromStdString(e.relative_path), qint64(e.size),
                            QString::fromStdString(std::string(hoardor::file::to_string(e.kind))), e.unsettled,
                            e.mtime_ns / 1'000'000});
    }
    endInsertRows();
    emit countChanged();
}

}
