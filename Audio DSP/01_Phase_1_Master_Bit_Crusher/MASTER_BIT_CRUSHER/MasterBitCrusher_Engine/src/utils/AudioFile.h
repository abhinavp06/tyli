#pragma once
#include <vector>
#include <string>

// We will use dr_wav / dr_flac here
struct AudioBuffer {
    std::vector<double> samples; // Interleaved if stereo
    int channels;
    int sampleRate;
};

class AudioFile {
public:
    static AudioBuffer load(const std::string& path);
    static bool save(const std::string& path, const AudioBuffer& buffer);
};
