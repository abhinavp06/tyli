#pragma once

#include <hoardor/player/player.hpp>

#include <QObject>
#include <QStringList>
#include <QTimer>
#include <QVariant>
#include <QtQml/qqmlregistration.h>

#include <memory>
#include <optional>

class QQmlEngine;
class QJSEngine;

namespace tyli {

// What's playing, the queue, and the user's state per file, for QML as the `Player`
// singleton (features/player_ui.md §3). A second bridge object next to `Library`, because
// it holds state: hoardor's Player and its thread. Callbacks come on that thread and are
// handed to the UI thread with queued calls, as LibraryBridge does for SyncWorker.
class PlayerBridge : public QObject {
    Q_OBJECT
    QML_NAMED_ELEMENT(Player)
    QML_SINGLETON

    Q_PROPERTY(bool ready READ ready CONSTANT)                       // false: libmpv couldn't start
    Q_PROPERTY(QString state READ state NOTIFY statusChanged)        // idle | loading | playing | paused
    Q_PROPERTY(bool active READ active NOTIFY statusChanged)         // something is loaded
    Q_PROPERTY(qint64 entryId READ entryId NOTIFY statusChanged)     // what's loaded, or 0
    Q_PROPERTY(bool hasVideo READ hasVideo NOTIFY statusChanged)
    Q_PROPERTY(qint64 duration READ duration NOTIFY statusChanged)
    Q_PROPERTY(qint64 position READ position NOTIFY positionChanged)
    Q_PROPERTY(qint64 resumedFrom READ resumedFrom NOTIFY statusChanged)
    Q_PROPERTY(int volume READ volume NOTIFY statusChanged)
    Q_PROPERTY(bool muted READ muted NOTIFY statusChanged)
    Q_PROPERTY(QVariantList audioTracks READ audioTracks NOTIFY statusChanged)       // [{id, label, selected}]
    Q_PROPERTY(QVariantList subtitleTracks READ subtitleTracks NOTIFY statusChanged)
    // The loaded file, described for the now-playing bar.
    Q_PROPERTY(QString title READ title NOTIFY nowPlayingChanged)
    Q_PROPERTY(QString subtitle READ subtitle NOTIFY nowPlayingChanged)
    Q_PROPERTY(qint64 coverEntry READ coverEntry NOTIFY nowPlayingChanged)
    Q_PROPERTY(QVariantMap link READ link NOTIFY nowPlayingChanged)  // {categoryId, page, params}: its album, movie, or show
    Q_PROPERTY(int queueLength READ queueLength NOTIFY queueChanged)
    Q_PROPERTY(int current READ current NOTIFY queueChanged)
    // Bumped whenever likes, positions, or viewed marks may have changed: bindings that call
    // isLiked() or itemStates() read it so they re-evaluate.
    Q_PROPERTY(int revision READ revision NOTIFY statesChanged)

public:
    explicit PlayerBridge(const QString& databasePath, QObject* parent = nullptr);
    ~PlayerBridge() override;

    static void setInstance(PlayerBridge* bridge) { instance_ = bridge; }
    static PlayerBridge* instance() { return instance_; }
    static PlayerBridge* create(QQmlEngine* qmlEngine, QJSEngine* jsEngine);

    // C++ only: the video item draws through it (VideoItem).
    hoardor::player::Player* player() { return player_.get(); }

    bool ready() const { return player_ != nullptr; }
    QString state() const;
    bool active() const { return status_.state != hoardor::player::State::Idle; }
    qint64 entryId() const { return status_.entry.value_or(0); }
    bool hasVideo() const { return status_.has_video; }
    qint64 duration() const { return status_.duration_ms; }
    qint64 position() const { return position_; }
    qint64 resumedFrom() const { return status_.resumed_from_ms; }
    int volume() const { return status_.volume; }
    bool muted() const { return status_.muted; }
    QVariantList audioTracks() const;
    QVariantList subtitleTracks() const;
    QString title() const { return now_.title; }
    QString subtitle() const { return now_.subtitle; }
    qint64 coverEntry() const { return now_.coverEntry; }
    QVariantMap link() const { return now_.link; }
    int queueLength() const { return int(queue_.size()); }
    int current() const { return current_; }
    int revision() const { return revision_; }

    // ---- The queue (entry ids)
    Q_INVOKABLE void playNow(const QVariantList& entryIds, int start = 0);
    Q_INVOKABLE void add(const QVariantList& entryIds);
    Q_INVOKABLE void jump(int index);
    Q_INVOKABLE void remove(int index);
    Q_INVOKABLE void clear();
    QList<qint64> queue() const { return queue_; }

    // ---- Transport
    Q_INVOKABLE void toggle();
    Q_INVOKABLE void pause();
    Q_INVOKABLE void resume();
    Q_INVOKABLE void stop();
    Q_INVOKABLE void next();
    Q_INVOKABLE void previous();
    Q_INVOKABLE void seek(qint64 positionMs);
    Q_INVOKABLE void seekBy(qint64 deltaMs);
    Q_INVOKABLE void setVolume(int volume);
    Q_INVOKABLE void setMuted(bool muted);
    Q_INVOKABLE void selectAudio(int trackId);
    Q_INVOKABLE void selectSubtitle(int trackId);   // -1: off

    // ---- The user's state per file (the UI's own connection; never touches a drive)
    Q_INVOKABLE bool isLiked(qint64 entryId);
    Q_INVOKABLE void setLiked(qint64 entryId, bool liked);
    // [{entryId, position, duration, viewed, playCount, liked}] in the given order.
    Q_INVOKABLE QVariantList itemStates(const QVariantList& entryIds);

    // A file described for lists (the queue): {title, subtitle, duration, coverEntry, link}.
    struct Description {
        QString title, subtitle;
        qint64 duration = 0, coverEntry = 0;
        QVariantMap link;
    };
    Description describe(qint64 entryId) const;

signals:
    void statusChanged();
    void positionChanged();
    void nowPlayingChanged();
    void queueChanged();
    void statesChanged();

private:
    void onStatus(const hoardor::player::Status& status);
    void onQueue();
    void onError(qint64 entryId, const QString& message);
    void flushErrors();

    inline static PlayerBridge* instance_ = nullptr;
    std::unique_ptr<hoardor::player::Player> player_;
    std::optional<hoardor::db::Database> database_;          // the UI thread's connection for states and likes
    std::optional<hoardor::player::Library> items_;
    hoardor::player::Status status_;
    qint64 position_ = 0;
    Description now_;
    QList<qint64> queue_;
    int current_ = 0;
    int revision_ = 0;
    QTimer errorDelay_;                                      // several skipped files: one status line
    QStringList errors_;
};

}
