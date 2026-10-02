#include "category_files_model.hpp"

#include "library_bridge.hpp"

namespace tyli {

CategoryFilesModel::CategoryFilesModel(QObject* parent) : QAbstractListModel(parent) {
    if (LibraryBridge* bridge = LibraryBridge::instance()) {
        connect(bridge, &LibraryBridge::foldersChanged, this, &CategoryFilesModel::reload);
    }
}

void CategoryFilesModel::setCategoryId(int categoryId) {
    if (categoryId_ == categoryId) return;
    categoryId_ = categoryId;
    emit categoryIdChanged();
    reload();
}

void CategoryFilesModel::reload() {
    beginResetModel();
    rows_.clear();
    rows_.shrink_to_fit();
    folders_.clear();
    folderIndex_ = 0;
    after_ = 0;
    LibraryBridge* bridge = LibraryBridge::instance();
    hoardor::file::Library* library = bridge ? bridge->library() : nullptr;
    if (library && categoryId_ >= 0) {
        if (auto roots = library->roots(categoryId_)) {
            for (const auto& root : *roots) {
                folders_.push_back(Folder{root.id, QString::fromStdString(root.name),
                                          root.status != hoardor::file::RootStatus::Offline});
            }
        }
    }
    atEnd_ = folders_.empty();
    endResetModel();
    emit countChanged();
}

int CategoryFilesModel::rowCount(const QModelIndex& parent) const { return parent.isValid() ? 0 : int(rows_.size()); }

QVariant CategoryFilesModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= int(rows_.size())) return {};
    const Row& row = rows_[std::size_t(index.row())];
    switch (role) {
    case RelativePathRole:
    case Qt::DisplayRole: return row.relativePath;
    case FolderNameRole: return folders_[std::size_t(row.folder)].name;
    case FolderOnlineRole: return folders_[std::size_t(row.folder)].online;
    case SizeRole: return row.size;
    case KindRole: return row.kind;
    case UnsettledRole: return row.unsettled;
    case MtimeRole: return row.mtimeMs;
    default: return {};
    }
}

QHash<int, QByteArray> CategoryFilesModel::roleNames() const {
    return {{RelativePathRole, "relativePath"}, {FolderNameRole, "folderName"}, {FolderOnlineRole, "folderOnline"},
            {SizeRole, "size"},                 {KindRole, "kind"},             {UnsettledRole, "unsettled"},
            {MtimeRole, "mtime"}};
}

bool CategoryFilesModel::canFetchMore(const QModelIndex& parent) const { return !parent.isValid() && !atEnd_; }

void CategoryFilesModel::fetchMore(const QModelIndex& parent) {
    if (parent.isValid() || atEnd_) return;
    LibraryBridge* bridge = LibraryBridge::instance();
    hoardor::file::Library* library = bridge ? bridge->library() : nullptr;
    if (!library) {
        atEnd_ = true;
        return;
    }

    // Fill one page, moving on to the next folder when one runs out. Keyset paging
    // (after the last id seen), so every page costs the same however deep you scroll.
    std::vector<Row> page;
    while (int(page.size()) < PageSize && folderIndex_ < folders_.size()) {
        const std::size_t want = std::size_t(PageSize) - page.size();
        auto entries = library->entries(folders_[folderIndex_].id, after_, want);
        if (!entries) {
            emit bridge->errorOccurred(QString::fromStdString(entries.error().message));
            atEnd_ = true;
            break;
        }
        for (const auto& e : *entries) {
            page.push_back(Row{e.id, QString::fromStdString(e.relative_path), qint64(e.size),
                               QString::fromStdString(std::string(hoardor::file::to_string(e.kind))),
                               e.mtime_ns / 1'000'000, int(folderIndex_), e.unsettled});
        }
        if (entries->size() < want) {
            ++folderIndex_;
            after_ = 0;
        } else {
            after_ = entries->back().id;
        }
    }
    if (folderIndex_ >= folders_.size()) atEnd_ = true;
    if (page.empty()) return;

    const int first = int(rows_.size());
    beginInsertRows({}, first, first + int(page.size()) - 1);
    rows_.insert(rows_.end(), std::make_move_iterator(page.begin()), std::make_move_iterator(page.end()));
    endInsertRows();
    emit countChanged();
}

}
