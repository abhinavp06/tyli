#include <iostream>
#include <string>
#include "../utils/audio_file.h"
#include "../dsp/filter_bank.h"
#include "../dsp/Resampler.h"
#include "../dsp/Quantizer.h"
#include "../dsp/Dither.h"
#include "../dsp/Dynamics.h"

struct Config {
    // Core settings
    double bitDepth = 8.0;
    int rateDivisor = 1;  // 1 = no decimation, 10 = heavy lo-fi

    // Modes
    bool useDynamicBitDepth = false;
    bool useBoxcarResampling = true;  // true = Boxcar, false = Naive
    bool enableDither = true;
    bool enableNoiseShaping = false;

    // Dynamics parameters
    double baseBitDepth = 4.0;      // Minimum bit depth when quiet
    double modulationDepth = 12.0;  // Maximum added bits when loud
    double attackMs = 5.0;
    double releaseMs = 200.0;
};

int main(int argc, char* argv[]) {
    std::cout << "=== MASTER BIT CRUSHER v1.0 ===" << std::endl;

    // Configuration
    Config config;

    // Parse command line (simple version)
    std::string inputFile = "input.flac";
    std::string outputFile = "output_crushed.wav";

    if (argc >= 2) inputFile = argv[1];
    if (argc >= 3) outputFile = argv[2];

    // User options (you can modify these)
    std::cout << "\n--- CONFIGURATION ---" << std::endl;
    std::cout << "Dynamic Bit Depth? (0=No, 1=Yes): ";
    int dynChoice;
    std::cin >> dynChoice;
    config.useDynamicBitDepth = (dynChoice == 1);

    if (config.useDynamicBitDepth) {
        std::cout << "Base Bit Depth (1-16): ";
        std::cin >> config.baseBitDepth;
        std::cout << "Max Modulation Depth (0-16): ";
        std::cin >> config.modulationDepth;
    }
    else {
        std::cout << "Static Bit Depth (1-16): ";
        std::cin >> config.bitDepth;
    }

    std::cout << "Sample Rate Divisor (1=None, 10=Heavy): ";
    std::cin >> config.rateDivisor;

    std::cout << "Use Boxcar (Anti-Aliased)? (0=Naive, 1=Boxcar): ";
    int boxcarChoice;
    std::cin >> boxcarChoice;
    config.useBoxcarResampling = (boxcarChoice == 1);

    std::cout << "\n--- PROCESSING ---" << std::endl;

    // Load audio
    std::cout << "Loading: " << inputFile << "..." << std::endl;
    AudioBuffer audio = AudioFile::load(inputFile);

    if (audio.samples.empty()) {
        std::cerr << "Failed to load audio file!" << std::endl;
        return 1;
    }

    std::cout << "Loaded: " << audio.samples.size() << " samples, "
        << audio.channels << " channels, "
        << audio.sampleRate << " Hz" << std::endl;

    // Initialize DSP blocks
    FilterBank filters;
    filters.init(audio.sampleRate);

    Resampler resampler;
    resampler.setRateDivisor(config.rateDivisor);
    resampler.setJitterAmount(0.0); // No jitter for now (can add later)
    resampler.setMode(config.useBoxcarResampling ? Resampler::Mode::Boxcar : Resampler::Mode::Naive);

    Quantizer quantizer;
    quantizer.setArchitecture(Quantizer::Architecture::MidTread);

    Dither dither;
    dither.setNoiseShaping(config.enableNoiseShaping);

    EnvelopeFollower envelope;
    if (config.useDynamicBitDepth) {
        envelope.setAttackRelease(config.attackMs, config.releaseMs, audio.sampleRate);
    }

    // Process samples
    std::cout << "Processing..." << std::endl;

    for (size_t i = 0; i < audio.samples.size(); ++i) {
        double sample = audio.samples[i];

        // 1. DC Blocker
        sample = filters.processDCBlocker(sample);

        // 2. Envelope Detection (if dynamic mode)
        double currentBitDepth = config.bitDepth;
        if (config.useDynamicBitDepth) {
            double env = envelope.process(sample);
            // Linear mapping: baseBits + (envelope * modulationDepth)
            currentBitDepth = config.baseBitDepth + (env * config.modulationDepth);
            currentBitDepth = std::clamp(currentBitDepth, 1.0, 32.0);
        }

        // 3. Resampler (Sample Rate Reduction)
        sample = resampler.process(sample);

        // 4. Anti-Alias Filter
        if (config.rateDivisor > 1) {
            sample = filters.processAntiAlias(sample);
        }

        // 5. Dither (if enabled)
        if (config.enableDither) {
            dither.setAmplitude(currentBitDepth);
            double ditherNoise = dither.process(0.0, sample); // Simplified call
            sample += ditherNoise;
        }

        // 6. Quantizer (Bit Depth Reduction)
        quantizer.setBitDepth(currentBitDepth);
        sample = quantizer.process(sample);

        // Write back
        audio.samples[i] = sample;

        // Progress indicator
        if (i % (audio.samples.size() / 10) == 0) {
            std::cout << "." << std::flush;
        }
    }

    std::cout << "\nDone!" << std::endl;

    // Save output
    std::cout << "Saving: " << outputFile << "..." << std::endl;
    if (AudioFile::save(outputFile, audio)) {
        std::cout << "Success! File saved." << std::endl;
    }
    else {
        std::cerr << "Failed to save file!" << std::endl;
        return 1;
    }

    return 0;
}