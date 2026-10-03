#include "video_item.hpp"

#include "player_bridge.hpp"

#include <QOpenGLContext>
#include <QOpenGLFramebufferObject>
#include <QPointer>

namespace tyli {

namespace {

void* procAddress(void*, const char* name) {
    QOpenGLContext* context = QOpenGLContext::currentContext();
    return context ? reinterpret_cast<void*>(context->getProcAddress(name)) : nullptr;
}

class MpvRenderer : public QQuickFramebufferObject::Renderer {
public:
    explicit MpvRenderer(VideoItem* item) : item_(item) {}

    // On the render thread, with the OpenGL context current: mpv's renderer must go here.
    ~MpvRenderer() override {
        if (video_) {
            video_->set_update_callback(nullptr);
            video_->destroy();
        }
    }

    void render() override {
        if (!video_) {
            auto* bridge = PlayerBridge::instance();
            if (!bridge || !bridge->player()) return;
            auto& video = bridge->player()->video();
            QPointer<VideoItem> item = item_;
            // mpv calls this from its own thread when a frame is ready: schedule a redraw.
            video.set_update_callback([item] {
                if (item) QMetaObject::invokeMethod(item, "update", Qt::QueuedConnection);
            });
            if (!video.create(&procAddress, nullptr)) return;
            video_ = &video;
            QMetaObject::invokeMethod(item, [item] {
                if (item) item->setReady();
            }, Qt::QueuedConnection);
        }
        QOpenGLFramebufferObject* fbo = framebufferObject();
        video_->render(int(fbo->handle()), fbo->width(), fbo->height(), false);
    }

private:
    VideoItem* item_;
    hoardor::player::VideoRenderer* video_ = nullptr;
};

}

VideoItem::VideoItem(QQuickItem* parent) : QQuickFramebufferObject(parent) {}

QQuickFramebufferObject::Renderer* VideoItem::createRenderer() const { return new MpvRenderer(const_cast<VideoItem*>(this)); }

}
