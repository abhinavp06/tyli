#pragma once

// QML names for hoardor's query fields ("albumArtist" -> audio::Field::AlbumArtist), and
// QML filter maps -> hoardor filters. One place, so QML never sees hoardor's enums.

#include <hoardor/audio/audio.hpp>
#include <hoardor/video/video.hpp>

#include <QString>
#include <QStringList>
#include <QVariantMap>

#include <optional>
#include <vector>

namespace tyli::fields {

std::optional<hoardor::audio::Field> audio(const QString& name);
std::optional<hoardor::video::Field> video(const QString& name);

// {fieldName: value}; numbers stay numbers, everything else is text. Unknown names: nullopt.
std::optional<hoardor::audio::Filter> audioFilter(const QVariantMap& filter);
std::optional<hoardor::video::Filter> videoFilter(const QVariantMap& filter);

// ["disc", "track"] or ["-height"] (a leading '-' is descending).
std::optional<std::vector<hoardor::audio::Order>> audioOrder(const QStringList& order);
std::optional<std::vector<hoardor::video::Order>> videoOrder(const QStringList& order);

// The entry a movie's or show's poster thumbnail is named after: its poster image, else the video
// with a poster inside, else its first video (Plex's poster or a frame; ThumbnailCache).
inline qint64 videoCover(const hoardor::video::Group& g) {
    return g.poster_entry ? g.poster_entry : (g.embedded_poster_entry ? g.embedded_poster_entry : g.first_entry);
}

}
