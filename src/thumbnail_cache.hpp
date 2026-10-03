#pragma once

#include <hoardor/file/library.hpp>

#include <QHash>
#include <QObject>
#include <QString>

namespace tyli {

// Small cover and poster images on internal storage, so browsing never wakes a drive
// (features/media_listing_ui.md §3, decision D1: TYLI owns thumbnails).
// - rebuild() runs after each metadata pass, while the drives are awake: on Qt's thread
//   pool with its own hoardor connection, it makes a thumbnail for every album, movie, and
//   show whose art has none yet, and removes thumbnails nothing uses any more.
// - url() answers from the cache only (SQLite + one file check), on the UI thread.
// A thumbnail is named after the art's file entry and its mtime, so changed art gets a new one.
class ThumbnailCache : public QObject {
    Q_OBJECT

public:
    static constexpr int Width = 300;  // pixels; height follows the image (square covers, 2:3 posters)

    ThumbnailCache(const QString& folder, const QString& databasePath, hoardor::file::Library* uiLibrary,
                   QObject* parent = nullptr);

    QString folder() const { return folder_; }
    // A file: URL for the entry's thumbnail, or "" if there is none (yet).
    QString url(qint64 entryId);
    // Starts a background pass; if one is running, another runs after it.
    void rebuild();
    bool running() const { return running_; }

    struct Result {
        int made = 0, failed = 0, removed = 0;
    };
    // One pass, synchronously (rebuild() runs it on the thread pool; tests call it directly).
    static Result build(const QString& folder, const QString& databasePath);
    static QString fileName(qint64 entryId, qint64 mtimeNs);

signals:
    void changed();  // thumbnails were added or removed: models refresh their thumbnail role
    void runningChanged();

private:
    QString folder_;
    QString databasePath_;
    hoardor::file::Library* uiLibrary_;
    QHash<qint64, QString> known_;  // entry -> url ("" = none), cleared on changed()
    bool running_ = false;
    bool again_ = false;
};

}
