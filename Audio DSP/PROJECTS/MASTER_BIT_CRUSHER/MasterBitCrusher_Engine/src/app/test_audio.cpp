#include "../utils/audio_file.h"
#include <iostream>

int main() {
    std::cout << "Testing WAV..." << std::endl;
    auto wavBuffer = AudioFile::load("tests/input.wav");
    if (!wavBuffer.samples.empty()) {
        AudioFile::save("tests/output_from_wav.wav", wavBuffer);
    }

    std::cout << "Testing FLAC..." << std::endl;
    auto flacBuffer = AudioFile::load("tests/input.flac");
    if (!flacBuffer.samples.empty()) {
        AudioFile::save("tests/output_from_flac.wav", flacBuffer);
    }

    return 0;
}