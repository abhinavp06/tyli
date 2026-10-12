#include "copy_grouper.hpp"

#include <QTest>

using namespace tyli;
namespace audio = hoardor::audio;
namespace video = hoardor::video;

namespace {

audio::Track track(int disc, int number, bool lossless, int depth, int rate, int kbps, const std::string& codec, qint64 size = 100) {
    audio::Track t;
    t.disc = disc;
    t.track = number;
    t.title = "T" + std::to_string(number);
    t.lossless = lossless;
    t.bit_depth = depth;
    t.sample_rate = rate;
    t.bitrate_kbps = kbps;
    t.codec = codec;
    t.size = std::uint64_t(size);
    t.root_online = true;
    return t;
}

video::Item item(int height, const std::string& hdr, const std::string& codec, bool online = true, int season = -1, int episode = 0) {
    video::Item i;
    i.height = height;
    i.width = height * 16 / 9;
    i.hdr = hdr;
    i.video_codec = codec;
    i.root_online = online;
    i.season = season;
    i.episode = episode;
    return i;
}

}

class TestCopyGrouper : public QObject {
    Q_OBJECT

private slots:
    void audioLabels() {
        QCOMPARE(copies::audioLabel(track(1, 1, true, 24, 96000, 2500, "flac")), QStringLiteral("24/96 FLAC"));
        QCOMPARE(copies::audioLabel(track(1, 1, true, 16, 44100, 900, "alac")), QStringLiteral("16/44.1 ALAC"));
        QCOMPARE(copies::audioLabel(track(1, 1, true, 24, 48000, 2300, "pcm_s24le")), QStringLiteral("24/48 PCM"));
        QCOMPARE(copies::audioLabel(track(1, 1, false, 0, 44100, 319, "mp3")), QStringLiteral("MP3 320"));
        QCOMPARE(copies::audioLabel(track(1, 1, false, 0, 44100, 245, "mp3")), QStringLiteral("MP3 250"));  // VBR: rounded
        QCOMPARE(copies::audioLabel(track(1, 1, false, 0, 44100, 256, "aac")), QStringLiteral("AAC 256"));
    }

    void albumCopiesBestFirstWithPartialCopies() {
        std::vector<audio::Track> all;
        for (int n = 1; n <= 4; ++n) all.push_back(track(1, n, true, 16, 44100, 900, "flac"));
        for (int n = 4; n >= 1; --n) all.push_back(track(1, n, true, 24, 96000, 2500, "flac", 300));  // out of order
        for (int n = 1; n <= 3; ++n) all.push_back(track(1, n, false, 0, 44100, 320, "mp3"));       // one track missing
        const auto c = copies::groupAudio(all);
        QCOMPARE(c.size(), std::size_t(3));
        QCOMPARE(c[0].label, QStringLiteral("24/96 FLAC"));
        QCOMPARE(c[1].label, QStringLiteral("16/44.1 FLAC"));
        QCOMPARE(c[2].label, QStringLiteral("MP3 320"));
        QCOMPARE(c[0].tracks.front().track, 1);  // sorted by disc, then number
        QCOMPARE(c[0].size, 1200);
        QVERIFY(c[0].complete());
        QCOMPARE(c[2].totalTracks, 4);
        QVERIFY(!c[2].complete());
    }

    void discsAndUntaggedTracks() {
        std::vector<audio::Track> all{track(2, 1, true, 16, 44100, 900, "flac"), track(1, 2, true, 16, 44100, 900, "flac"),
                                      track(1, 1, true, 16, 44100, 900, "flac")};
        auto untagged = track(0, 0, true, 16, 44100, 900, "flac");
        untagged.title = "Bonus";
        all.push_back(untagged);
        const auto c = copies::groupAudio(all);
        QCOMPARE(c.size(), std::size_t(1));
        QCOMPARE(c[0].totalTracks, 4);
        QCOMPARE(c[0].tracks[0].disc, 0);  // untagged first (disc 0), then 1/1, 1/2, 2/1
        QCOMPARE(c[0].tracks[3].disc, 2);
    }

    void videoLabelsAndOrder() {
        QCOMPARE(copies::videoLabel(item(2160, "HDR10", "hevc")), QStringLiteral("2160p · HDR10 · HEVC"));
        QCOMPARE(copies::videoLabel(item(1080, "", "h264")), QStringLiteral("1080p · SDR · H.264"));
        const auto c = copies::groupVideo({item(1080, "", "h264"), item(2160, "", "hevc"), item(2160, "Dolby Vision", "hevc"),
                                           item(2160, "HDR10", "hevc"), item(720, "", "h264")});
        QCOMPARE(c.size(), std::size_t(5));
        QCOMPARE(c[0].label, QStringLiteral("2160p · Dolby Vision · HEVC"));
        QCOMPARE(c[1].label, QStringLiteral("2160p · HDR10 · HEVC"));
        QCOMPARE(c[2].label, QStringLiteral("2160p · SDR · HEVC"));
        QCOMPARE(c.back().label, QStringLiteral("720p · SDR · H.264"));
    }

    void theSameQualityTwiceIsOneOptionAndTheOnlineOnePlays() {
        const auto c = copies::groupVideo({item(1080, "", "h264", false), item(1080, "", "h264", true)});
        QCOMPARE(c.size(), std::size_t(1));
        QVERIFY(c[0].item.root_online);
    }

    void showQualitiesCountEpisodes() {
        std::vector<video::Item> all;
        for (int e = 1; e <= 4; ++e) all.push_back(item(2160, "HDR10", "hevc", true, 1, e));
        for (int e = 1; e <= 2; ++e) all.push_back(item(1080, "", "h264", true, 1, e));
        all.push_back(item(1080, "", "h264", true, 2, 1));
        const auto show = copies::groupShow(all);
        QCOMPARE(show.episodes.size(), std::size_t(5));
        QCOMPARE(show.qualities.size(), std::size_t(2));
        QCOMPARE(show.qualities[0].label, QStringLiteral("2160p · HDR10 · HEVC"));
        QCOMPARE(show.qualities[0].episodes, 4);
        QCOMPARE(show.qualities[1].episodes, 3);
        QCOMPARE(show.episodes.back().season, 2);
        QCOMPARE(show.episodes.front().copies.size(), std::size_t(2));
    }

    void specialsWithoutANumberStayApartByTitle() {
        // A show's films and OVAs become specials numbered 0 (hoardor features/shows_and_extras.md).
        const auto special = [](const std::string& title, int height) {
            auto i = item(height, "", "h264", true, 0, 0);
            i.title = title;
            return i;
        };
        const auto show = copies::groupShow({special("Movie 02 - The World's Strongest", 1080), special("Movie 01 - Dead Zone", 1080),
                                             special("movie 01 - dead zone", 480), item(1080, "", "h264", true, 1, 1)});
        QCOMPARE(show.episodes.size(), std::size_t(3));
        QCOMPARE(show.episodes[0].season, 0);
        QCOMPARE(show.episodes[0].copies.size(), std::size_t(2));  // the same film twice: copies
        QCOMPARE(QString::fromStdString(show.episodes[0].copies[0].item.title), QStringLiteral("Movie 01 - Dead Zone"));
        QCOMPARE(QString::fromStdString(show.episodes[1].copies[0].item.title), QStringLiteral("Movie 02 - The World's Strongest"));
        QCOMPARE(show.episodes[2].season, 1);
    }
};

QTEST_MAIN(TestCopyGrouper)
#include "tst_copy_grouper.moc"
