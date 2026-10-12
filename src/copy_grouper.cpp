#include "copy_grouper.hpp"

#include <algorithm>
#include <cstdlib>
#include <map>
#include <tuple>

namespace tyli::copies {

namespace audio = hoardor::audio;
namespace video = hoardor::video;

namespace {

QString kilohertz(int rate) {
    if (rate <= 0) return QStringLiteral("?");
    QString text = QString::number(rate / 1000.0, 'f', 1);
    if (text.endsWith(QLatin1String(".0"))) text.chop(2);
    return text;
}

QString codecName(const std::string& codec) {
    const QString c = QString::fromStdString(codec);
    if (c.startsWith(QLatin1String("pcm_"))) return QStringLiteral("PCM");
    if (c.startsWith(QLatin1String("dsd_"))) return QStringLiteral("DSD");
    if (c == QLatin1String("wavpack")) return QStringLiteral("WV");
    if (c == QLatin1String("h264")) return QStringLiteral("H.264");
    if (c == QLatin1String("vorbis")) return QStringLiteral("OGG");
    return c.toUpper();
}

// Common constant bit rates; anything else (VBR) is rounded to 10.
int nominalBitrate(int kbps) {
    for (int nominal : {64, 96, 128, 160, 192, 224, 256, 320}) {
        if (std::abs(kbps - nominal) * 100 <= nominal * 4) return nominal;
    }
    return (kbps + 5) / 10 * 10;
}

int hdrRank(const std::string& hdr) {
    if (hdr == "Dolby Vision") return 3;
    if (hdr == "HDR10") return 2;
    if (hdr == "HLG") return 1;
    return 0;
}

int codecRank(const std::string& codec) {
    if (codec == "av1") return 4;
    if (codec == "hevc") return 3;
    if (codec == "vp9") return 2;
    if (codec == "h264") return 1;
    return 0;
}

QString resolution(int height) {
    if (height >= 2000) return QStringLiteral("2160p");
    if (height >= 1400) return QStringLiteral("1440p");
    if (height >= 1000) return QStringLiteral("1080p");
    if (height >= 700) return QStringLiteral("720p");
    return QString::number(height) + QLatin1Char('p');
}

QString trackIdentity(const audio::Track& t) {
    if (t.track > 0) return QStringLiteral("%1/%2").arg(t.disc).arg(t.track);
    return QString::fromStdString(t.title).toLower();
}

auto audioRank(const audio::Track& t) { return std::make_tuple(t.lossless, t.bit_depth, t.sample_rate, nominalBitrate(t.bitrate_kbps)); }

auto videoRank(const video::Item& i) {
    return std::make_tuple(i.height, hdrRank(i.hdr), codecRank(i.video_codec), static_cast<qint64>(i.size));
}

}

QString audioLabel(const audio::Track& t) {
    if (t.lossless) {
        const QString depth = t.bit_depth > 0 ? QString::number(t.bit_depth) : QStringLiteral("?");
        return depth + QLatin1Char('/') + kilohertz(t.sample_rate) + QLatin1Char(' ') + codecName(t.codec);
    }
    return codecName(t.codec) + QLatin1Char(' ') + QString::number(nominalBitrate(t.bitrate_kbps));
}

QString videoLabel(const video::Item& i) {
    return resolution(i.height) + QStringLiteral(" · ") + (i.hdr.empty() ? QStringLiteral("SDR") : QString::fromStdString(i.hdr)) +
           QStringLiteral(" · ") + codecName(i.video_codec);
}

std::vector<AudioCopy> groupAudio(std::vector<audio::Track> tracks) {
    std::map<QString, AudioCopy> byLabel;
    std::map<QString, decltype(audioRank(tracks.front()))> rank;
    std::vector<QString> identities;
    for (auto& t : tracks) {
        identities.push_back(trackIdentity(t));
        const QString label = audioLabel(t);
        AudioCopy& copy = byLabel[label];
        copy.label = label;
        copy.size += static_cast<qint64>(t.size);
        rank.emplace(label, audioRank(t));
        copy.tracks.push_back(std::move(t));
    }
    std::sort(identities.begin(), identities.end());
    const int total = int(std::unique(identities.begin(), identities.end()) - identities.begin());

    std::vector<AudioCopy> out;
    for (auto& [label, copy] : byLabel) {
        copy.totalTracks = total;
        std::sort(copy.tracks.begin(), copy.tracks.end(), [](const audio::Track& a, const audio::Track& b) {
            return std::tie(a.disc, a.track, a.title) < std::tie(b.disc, b.track, b.title);
        });
        out.push_back(std::move(copy));
    }
    std::sort(out.begin(), out.end(), [&](const AudioCopy& a, const AudioCopy& b) {
        if (rank.at(a.label) != rank.at(b.label)) return rank.at(a.label) > rank.at(b.label);
        return a.tracks.size() > b.tracks.size();
    });
    return out;
}

std::vector<VideoCopy> groupVideo(std::vector<video::Item> items) {
    std::sort(items.begin(), items.end(), [](const video::Item& a, const video::Item& b) {
        if (videoRank(a) != videoRank(b)) return videoRank(a) > videoRank(b);
        return a.root_online > b.root_online;  // same quality twice: the reachable one first
    });
    std::vector<VideoCopy> out;
    for (auto& i : items) {
        const QString label = videoLabel(i);
        // The same quality on two drives is one option; the first (online) one plays.
        if (std::any_of(out.begin(), out.end(), [&](const VideoCopy& c) { return c.label == label; })) continue;
        out.push_back(VideoCopy{label, std::move(i)});
    }
    return out;
}

ShowCopies groupShow(std::vector<video::Item> episodes) {
    // An episode is its (season, number). Without a number (specials made of a show's films and
    // OVAs, all numbered 0), its title tells them apart, and orders them.
    std::map<std::tuple<int, int, QString>, std::vector<video::Item>> byEpisode;
    for (auto& e : episodes) {
        const QString title = e.episode > 0 ? QString() : QString::fromStdString(e.title).toCaseFolded();
        byEpisode[{e.season, e.episode, title}].push_back(std::move(e));
    }

    ShowCopies out;
    std::map<QString, std::pair<decltype(videoRank(video::Item{})), int>> qualities;
    for (auto& [key, items] : byEpisode) {
        Episode episode{std::get<0>(key), std::get<1>(key), groupVideo(std::move(items))};
        for (const auto& c : episode.copies) {
            auto& q = qualities[c.label];
            q.first = std::max(q.first, videoRank(c.item));
            ++q.second;
        }
        out.episodes.push_back(std::move(episode));
    }
    for (const auto& [label, q] : qualities) out.qualities.push_back(ShowQuality{label, q.second});
    std::sort(out.qualities.begin(), out.qualities.end(), [&](const ShowQuality& a, const ShowQuality& b) {
        return qualities.at(a.label).first > qualities.at(b.label).first;
    });
    return out;
}

}
