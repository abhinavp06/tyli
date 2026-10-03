#pragma once

// Copies of the same album, movie, or episode in different qualities (16/44.1 and 24/96
// FLAC, 1080p and 4K) are grouped here, in TYLI: hoardor never groups copies
// (features/media_listing_ui.md §3). Plain C++ on hoardor's types; tested on its own.

#include <hoardor/audio/audio.hpp>
#include <hoardor/video/video.hpp>

#include <QString>

#include <vector>

namespace tyli::copies {

// "24/96 FLAC", "16/44.1 ALAC", "MP3 320", "AAC 256".
QString audioLabel(const hoardor::audio::Track& track);
// "2160p · HDR10 · HEVC", "1080p · SDR · H.264".
QString videoLabel(const hoardor::video::Item& item);

struct AudioCopy {
    QString label;
    std::vector<hoardor::audio::Track> tracks;  // by disc, then track number, then title
    int totalTracks = 0;                        // distinct tracks across every copy of the album
    qint64 size = 0;
    bool complete() const { return int(tracks.size()) >= totalTracks; }
};

// An album's tracks (any order) as copies, best first: lossless before lossy, then bit
// depth, sample rate, and bit rate. A track's identity is (disc, track number), or its
// title when untagged.
std::vector<AudioCopy> groupAudio(std::vector<hoardor::audio::Track> tracks);

struct VideoCopy {
    QString label;
    hoardor::video::Item item;
};

// One movie's or one episode's files as copies, best first: resolution, then HDR
// (Dolby Vision > HDR10 > HLG > SDR), then codec (AV1 > HEVC > VP9 > H.264), then size.
std::vector<VideoCopy> groupVideo(std::vector<hoardor::video::Item> items);

// A show's quality options: each label with how many of the show's episodes have it.
struct ShowQuality {
    QString label;
    int episodes = 0;
};

struct Episode {
    int season = -1, episode = 0;
    std::vector<VideoCopy> copies;  // best first
};

struct ShowCopies {
    std::vector<ShowQuality> qualities;  // best first
    std::vector<Episode> episodes;       // by season, then episode
};

ShowCopies groupShow(std::vector<hoardor::video::Item> episodes);

}
