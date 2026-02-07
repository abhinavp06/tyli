#include "audio_file.h"
#include <iostream>
#include <vector>
#include <algorithm>

#define DR_FLAC_IMPLEMENTATION
#include "dr_flac.h"
#define DR_WAV_IMPLEMENTATION
#include "dr_wav.h"

static bool hasExtension(const std::string& filename, const std::string& ext) {
    if (filename.length() < ext.length()) {
        return false;
    }

    std::string fileExt = filename.substr(filename.length() - ext.length());

    std::transform(fileExt.begin(), fileExt.end(), fileExt.begin(), ::tolower);

    return fileExt == ext;
}

AudioBuffer AudioFile::load(const std::string& fileName) {
    AudioBuffer buffer;

    if (hasExtension(fileName, ".flac")) {
        return loadFlac(fileName);
    }
    else if (hasExtension(fileName, ".wav")) {
        return loadWav(fileName);
    }
    else {
        std::cerr << "[Error] Unsupported file format: " << fileName << std::endl;
        return buffer;
    }
}

AudioBuffer AudioFile::loadFlac(const std::string& fileName) {
    AudioBuffer buffer;

    unsigned int channels;
    unsigned int sampleRate;
    drflac_uint64 totalPCMFrameCount;

    float* pSampleData = drflac_open_file_and_read_pcm_frames_f32(
        fileName.c_str(),
        &channels,
        &sampleRate,
        &totalPCMFrameCount,
        NULL
    );

    if (pSampleData == NULL) {
        std::cerr << "[Error] Could not open FLAC file: " << fileName << std::endl;
        return buffer;
    }

    buffer.channels = channels;
    buffer.sampleRate = sampleRate;

    size_t totalSamples = (size_t)(totalPCMFrameCount * channels);
    buffer.samples.resize(totalSamples);

    for (size_t i = 0; i < totalSamples; ++i) {
        buffer.samples[i] = static_cast<double>(pSampleData[i]);
    }

    drflac_free(pSampleData, NULL);

    std::cout << "[Load FLAC] " << fileName << " | Rate: " << sampleRate
        << " | Channels: " << channels
        << " | Samples: " << totalSamples << std::endl;

    return buffer;
}

AudioBuffer AudioFile::loadWav(const std::string& fileName) {
    AudioBuffer buffer;

    unsigned int channels;
    unsigned int sampleRate;
    drwav_uint64 totalPCMFrameCount;

    float* pSampleData = drwav_open_file_and_read_pcm_frames_f32(
        fileName.c_str(),
        &channels,
        &sampleRate,
        &totalPCMFrameCount,
        NULL
    );

    if (pSampleData == NULL) {
        std::cerr << "[Error] Could not open WAV file: " << fileName << std::endl;
        return buffer;
    }

    buffer.channels = channels;
    buffer.sampleRate = sampleRate;

    size_t totalSamples = (size_t)(totalPCMFrameCount * channels);
    buffer.samples.resize(totalSamples);

    for (size_t i = 0; i < totalSamples; ++i) {
        buffer.samples[i] = static_cast<double>(pSampleData[i]);
    }

    drwav_free(pSampleData, NULL);

    std::cout << "[Load WAV] " << fileName << " | Rate: " << sampleRate
        << " | Channels: " << channels
        << " | Samples: " << totalSamples << std::endl;

    return buffer;
}

bool AudioFile::save(const std::string& filename, const AudioBuffer& buffer) {
    drwav_data_format format;
    format.container = drwav_container_riff;
    format.format = DR_WAVE_FORMAT_IEEE_FLOAT;
    format.channels = buffer.channels;
    format.sampleRate = buffer.sampleRate;
    format.bitsPerSample = 32;

    drwav wav;
    if (!drwav_init_file_write(&wav, filename.c_str(), &format, NULL)) {
        std::cerr << "[Error] Could not open file for writing: " << filename << std::endl;
        return false;
    }

    std::vector<float> tempBuffer(buffer.samples.size());
    for (size_t i = 0; i < buffer.samples.size(); ++i) {
        double s = buffer.samples[i];
        if (s > 1.0) s = 1.0;
        if (s < -1.0) s = -1.0;
        tempBuffer[i] = static_cast<float>(s);
    }

    drwav_uint64 framesToWrite = buffer.samples.size() / buffer.channels;
    drwav_write_pcm_frames(&wav, framesToWrite, tempBuffer.data());

    drwav_uninit(&wav);

    std::cout << "[Save] Saved to " << filename << std::endl;
    return true;
}