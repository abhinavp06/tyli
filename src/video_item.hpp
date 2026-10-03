#pragma once

#include <QQuickFramebufferObject>
#include <QtQml/qqmlregistration.h>

namespace tyli {

// The video picture: mpv draws each frame into this item's OpenGL framebuffer through
// hoardor's VideoRenderer, on Qt's render thread (features/player_ui.md §3). It needs Qt
// Quick on OpenGL (main.cpp); without it (the software renderer in tests) QML doesn't create it.
// `ready` turns true once mpv's renderer exists: a video must not start before (mpv would
// play it without a picture).
class VideoItem : public QQuickFramebufferObject {
    Q_OBJECT
    QML_NAMED_ELEMENT(VideoView)
    Q_PROPERTY(bool ready READ ready NOTIFY readyChanged)

public:
    explicit VideoItem(QQuickItem* parent = nullptr);
    Renderer* createRenderer() const override;

    bool ready() const { return ready_; }
    void setReady() {
        if (ready_) return;
        ready_ = true;
        emit readyChanged();
    }

signals:
    void readyChanged();

private:
    bool ready_ = false;
};

}
