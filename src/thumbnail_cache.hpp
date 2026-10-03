#pragma once

#include <hoardor/file/library.hpp>

#include <QHash>
#include <QObject>
#include <QSet>
#include <QString>

namespace tyli {

// Small cover and poster images on internal storage, so browsing never wakes a drive
// (features/media_listing_ui.md §3, decision D1: TYLI owns thumbnails).
// - rebuild() runs after each metadata pass, while the drives are awake: on Qt's thread
//   pool with its own hoardor connection, it makes a thumbnail for every album, movie, and
//   show whose art has none yet, and removes thumbnails nothing uses any more.
// - url() answers from the cache only (SQLite + one file check), on the UI thread.
// A thumbnail is named after the art's file entry and its mtime, so changed art gets a new one.
// A movie or show with no art of its own gets Plex's poster, else a frame from its video
// (hoardor features/posters.md), named after that video's entry.
class ThumbnailCache : public QObject {
    Q_OBJECT

public:
    static constexpr int Width = 300;  // pixels; height follows the image (square covers, 2:3 posters)

    ThumbnailCache(const QString& folder, const QString& databasePath, hoardor::file::Library* uiLibrary,
                   QObject* parent = nullptr);

    QString folder() const { return folder_; }
    // A file: URL for the entry's thumbnail, or "" if there is none (yet).
    QString url(qint64 entryId);
    // Starts a background pass; if one is running, another runs after it. `frames`: also grab
    // frames from videos for posters (reads the drives: not while tags are being read).
    void rebuild(bool frames = true);
    bool running() const { return running_; }

    struct Options {
        bool frames = true;
        QString plexFolder;            // "" = find Plex on this machine (or $TYLI_PLEX_DIR)
        QSet<qint64> failedFrames;     // videos whose frame failed this session: not tried again
        QSet<qint64> plexMissed;       // videos Plex has no poster for (skipped in passes without frames)
    };
    struct Result {
        int made = 0, failed = 0, removed = 0;
        int fromPlex = 0, fromFrames = 0;
        QSet<qint64> failedFrames, plexMissed;
    };
    // One pass, synchronously (rebuild() runs it on the thread pool; tests call it directly).
    static Result build(const QString& folder, const QString& databasePath, const Options& options);
    static Result build(const QString& folder, const QString& databasePath);  // default Options
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
    bool againFrames_ = false;
    QSet<qint64> failedFrames_, plexMissed_;
};

}
