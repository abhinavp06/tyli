#pragma once
#include <vector>
#include <string>

struct AudioBuffer {
    std::vector<double> samples; // Interleaved if stereo
    int channels;
    int sampleRate;
};

class AudioFile {
public:
    static AudioBuffer load(const std::string& path);
    static bool save(const std::string& path, const AudioBuffer& buffer);

private:
    static AudioBuffer loadFlac(const std::string& fileName);
    static AudioBuffer loadWav(const std::string& fileName);
};
