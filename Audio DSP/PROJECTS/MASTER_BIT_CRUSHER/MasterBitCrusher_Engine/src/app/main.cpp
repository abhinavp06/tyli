#include <iostream>
#include <string>
#include "../utils/audio_file.h"
#include "../dsp/filter_bank.h"
#include "../dsp/resampler.h"
#include "../dsp/quantizer.h"
#include "../dsp/dither.h"
#include "../dsp/dynamics.h"
#include "../dsp/stereo_manager.h"

struct Config
{
    // Core settings
    double bitDepth = 8.0;
    int rateDivisor = 1; // 1 = no decimation, 10 = heavy lo-fi

    // Modes
    bool useDynamicBitDepth = false;
    bool useBoxcarResampling = true; // true = Boxcar, false = Naive
    bool enableDither = true;
    bool enableNoiseShaping = false;
    bool enableNoiseGate = true;

    StereoManager::Mode stereoMode = StereoManager::Mode::Linked;

    // Dynamics parameters
    double baseBitDepth = 4.0;     // Minimum bit depth when quiet
    double modulationDepth = 12.0; // Maximum added bits when loud
    double attackMs = 5.0;
    double releaseMs = 200.0;

    // Jitter
    double jitterAmount = 0.0; // (0.0 = off, 1.0 = max)
};

int main(int argc, char *argv[])
{
    std::cout << "=== MASTER BIT CRUSHER v1.0 ===" << std::endl;

    // Configuration
    Config config;

    // Parse command line (simple version)
    std::string inputFile = "input.flac";
    std::string outputFile = "output_crushed.wav";

    if (argc >= 2)
        inputFile = argv[1];
    if (argc >= 3)
        outputFile = argv[2];

    // User options (you can modify these)
    std::cout << "\n--- CONFIGURATION ---" << std::endl;
    std::cout << "Dynamic Bit Depth? (0=No, 1=Yes): ";
    int dynChoice;
    std::cin >> dynChoice;
    config.useDynamicBitDepth = (dynChoice == 1);

    if (config.useDynamicBitDepth)
    {
        std::cout << "Base Bit Depth (1-16): ";
        std::cin >> config.baseBitDepth;
        std::cout << "Max Modulation Depth (0-16): ";
        std::cin >> config.modulationDepth;
    }
    else
    {
        std::cout << "Static Bit Depth (1-16): ";
        std::cin >> config.bitDepth;
    }

    std::cout << "Sample Rate Divisor (1=None, 10=Heavy): ";
    std::cin >> config.rateDivisor;

    std::cout << "Use Boxcar (Anti-Aliased)? (0=Naive, 1=Boxcar): ";
    int boxcarChoice;
    std::cin >> boxcarChoice;
    config.useBoxcarResampling = (boxcarChoice == 1);

    std::cout << "Enable Noise Gate? (0=No, 1=Yes): ";
    int gateChoice;
    std::cin >> gateChoice;
    config.enableNoiseGate = (gateChoice == 1);

    std::cout << "Jitter Amount (0.0-1.0, 0=Off): ";
    std::cin >> config.jitterAmount;

    std::cout << "\n--- STEREO MODE ---" << std::endl;
    std::cout << "0 = Linked (Mono-like, correlated)" << std::endl;
    std::cout << "1 = Independent (Dual-mono, decorrelated dither)" << std::endl;
    std::cout << "2 = Mid-Side (Professional, decorrelated)" << std::endl;
    std::cout << "Choose mode (0-2): ";
    int stereoChoice;
    std::cin >> stereoChoice;

    if (stereoChoice == 0)
    {
        config.stereoMode = StereoManager::Mode::Linked;
    }
    else if (stereoChoice == 1)
    {
        config.stereoMode = StereoManager::Mode::Independent;
    }
    else
    {
        config.stereoMode = StereoManager::Mode::MidSide;
    }

    std::cout << "\n--- PROCESSING ---" << std::endl;

    // Load audio
    std::cout << "Loading: " << inputFile << "..." << std::endl;
    AudioBuffer audio = AudioFile::load(inputFile);

    if (audio.samples.empty())
    {
        std::cerr << "Failed to load audio file!" << std::endl;
        return 1;
    }

    std::cout << "Loaded: " << audio.samples.size() << " samples, "
              << audio.channels << " channels, "
              << audio.sampleRate << " Hz" << std::endl;

    bool isStereo = (audio.channels == 2);
    if (!isStereo && config.stereoMode != StereoManager::Mode::Linked)
    {
        std::cout << "[Warning] Mono file detected. Forcing Linked mode." << std::endl;
        config.stereoMode = StereoManager::Mode::Linked;
    }

    StereoManager stereo;
    stereo.setMode(config.stereoMode);

    // Initialize DSP blocks
    FilterBank filtersL;
    filtersL.init(audio.sampleRate);

    Resampler resamplerL;
    resamplerL.setRateDivisor(config.rateDivisor);
    resamplerL.setJitterAmount(config.jitterAmount);
    resamplerL.setMode(config.useBoxcarResampling ? Resampler::Mode::Boxcar : Resampler::Mode::Naive);

    Quantizer quantizerL;
    quantizerL.setArchitecture(Quantizer::Architecture::MidTread);

    Dither ditherL;
    ditherL.setSeed(12345); // Left/Mid channel seed
    ditherL.setNoiseShaping(config.enableNoiseShaping);
    ditherL.setGating(config.enableNoiseGate);

    EnvelopeFollower envelopeL;
    if (config.useDynamicBitDepth)
    {
        envelopeL.setAttackRelease(config.attackMs, config.releaseMs, audio.sampleRate);
    }

    FilterBank filtersR;
    Resampler resamplerR;
    Quantizer quantizerR;
    Dither ditherR;
    EnvelopeFollower envelopeR;

    if (isStereo && config.stereoMode != StereoManager::Mode::Linked)
    {
        filtersR.init(audio.sampleRate);

        resamplerR.setRateDivisor(config.rateDivisor);
        resamplerR.setJitterAmount(config.jitterAmount);
        resamplerR.setMode(config.useBoxcarResampling ? Resampler::Mode::Boxcar : Resampler::Mode::Naive);

        quantizerR.setArchitecture(Quantizer::Architecture::MidTread);

        ditherR.setSeed(67890); // Right/Side channel seed (DIFFERENT)
        ditherR.setNoiseShaping(config.enableNoiseShaping);
        ditherR.setGating(config.enableNoiseGate);

        if (config.useDynamicBitDepth)
        {
            envelopeR.setAttackRelease(config.attackMs, config.releaseMs, audio.sampleRate);
        }
    }

    // Process samples
    // Process samples
    std::cout << "Processing..." << std::endl;

    if (!isStereo)
    {
        // MONO PROCESSING (simple path)
        for (size_t i = 0; i < audio.samples.size(); ++i)
        {
            double sample = audio.samples[i];

            sample = filtersL.processDCBlocker(sample);

            double currentBitDepth = config.bitDepth;
            if (config.useDynamicBitDepth)
            {
                double env = envelopeL.process(sample);
                currentBitDepth = config.baseBitDepth + (env * config.modulationDepth);
                currentBitDepth = std::clamp(currentBitDepth, 1.0, 32.0);
            }

            sample = resamplerL.process(sample);

            if (config.rateDivisor > 1)
            {
                sample = filtersL.processAntiAlias(sample);
            }

            if (config.enableDither)
            {
                ditherL.setAmplitude(currentBitDepth);
                double ditherNoise = ditherL.process(0.0, sample);
                sample += ditherNoise;
            }

            quantizerL.setBitDepth(currentBitDepth);
            sample = quantizerL.process(sample);

            audio.samples[i] = sample;

            if (i % (audio.samples.size() / 10) == 0)
            {
                std::cout << "." << std::flush;
            }
        }
    }
    else
    {
        // STEREO PROCESSING
        size_t numFrames = audio.samples.size() / 2;

        for (size_t frame = 0; frame < numFrames; ++frame)
        {
            size_t idxL = frame * 2;
            size_t idxR = frame * 2 + 1;

            double left = audio.samples[idxL];
            double right = audio.samples[idxR];

            // === STEREO MODE BRANCH ===

            if (config.stereoMode == StereoManager::Mode::Linked)
            {
                // LINKED: Process left, copy to right
                left = filtersL.processDCBlocker(left);
                right = filtersL.processDCBlocker(right);

                double currentBitDepth = config.bitDepth;
                if (config.useDynamicBitDepth)
                {
                    double env = envelopeL.process((left + right) / 2.0); // Use average
                    currentBitDepth = config.baseBitDepth + (env * config.modulationDepth);
                    currentBitDepth = std::clamp(currentBitDepth, 1.0, 32.0);
                }

                left = resamplerL.process(left);
                right = resamplerL.process(right);

                if (config.rateDivisor > 1)
                {
                    left = filtersL.processAntiAlias(left);
                    right = filtersL.processAntiAlias(right);
                }

                if (config.enableDither)
                {
                    ditherL.setAmplitude(currentBitDepth);
                    double ditherNoise = ditherL.process(0.0, (left + right) / 2.0);
                    left += ditherNoise;
                    right += ditherNoise; // SAME dither (correlated)
                }

                quantizerL.setBitDepth(currentBitDepth);
                left = quantizerL.process(left);
                right = quantizerL.process(right);
            }

            else if (config.stereoMode == StereoManager::Mode::Independent)
            {
                // INDEPENDENT: Process L and R separately
                left = filtersL.processDCBlocker(left);
                right = filtersR.processDCBlocker(right);

                double currentBitDepthL = config.bitDepth;
                double currentBitDepthR = config.bitDepth;

                if (config.useDynamicBitDepth)
                {
                    double envL = envelopeL.process(left);
                    double envR = envelopeR.process(right);
                    currentBitDepthL = config.baseBitDepth + (envL * config.modulationDepth);
                    currentBitDepthR = config.baseBitDepth + (envR * config.modulationDepth);
                    currentBitDepthL = std::clamp(currentBitDepthL, 1.0, 32.0);
                    currentBitDepthR = std::clamp(currentBitDepthR, 1.0, 32.0);
                }

                left = resamplerL.process(left);
                right = resamplerR.process(right);

                if (config.rateDivisor > 1)
                {
                    left = filtersL.processAntiAlias(left);
                    right = filtersR.processAntiAlias(right);
                }

                if (config.enableDither)
                {
                    ditherL.setAmplitude(currentBitDepthL);
                    ditherR.setAmplitude(currentBitDepthR);
                    double ditherNoiseL = ditherL.process(0.0, left);
                    double ditherNoiseR = ditherR.process(0.0, right);
                    left += ditherNoiseL;  // Different dither
                    right += ditherNoiseR; // Different dither
                }

                quantizerL.setBitDepth(currentBitDepthL);
                quantizerR.setBitDepth(currentBitDepthR);
                left = quantizerL.process(left);
                right = quantizerR.process(right);
            }

            else
            { // MidSide mode
                // ENCODE L/R → M/S
                double mid, side;
                stereo.encode(left, right, mid, side);

                // Process MID
                mid = filtersL.processDCBlocker(mid);

                double currentBitDepthM = config.bitDepth;
                if (config.useDynamicBitDepth)
                {
                    double envM = envelopeL.process(mid);
                    currentBitDepthM = config.baseBitDepth + (envM * config.modulationDepth);
                    currentBitDepthM = std::clamp(currentBitDepthM, 1.0, 32.0);
                }

                mid = resamplerL.process(mid);

                if (config.rateDivisor > 1)
                {
                    mid = filtersL.processAntiAlias(mid);
                }

                if (config.enableDither)
                {
                    ditherL.setAmplitude(currentBitDepthM);
                    double ditherNoiseM = ditherL.process(0.0, mid);
                    mid += ditherNoiseM;
                }

                quantizerL.setBitDepth(currentBitDepthM);
                mid = quantizerL.process(mid);

                // Process SIDE
                side = filtersR.processDCBlocker(side);

                double currentBitDepthS = config.bitDepth;
                if (config.useDynamicBitDepth)
                {
                    double envS = envelopeR.process(side);
                    currentBitDepthS = config.baseBitDepth + (envS * config.modulationDepth);
                    currentBitDepthS = std::clamp(currentBitDepthS, 1.0, 32.0);
                }

                side = resamplerR.process(side);

                if (config.rateDivisor > 1)
                {
                    side = filtersR.processAntiAlias(side);
                }

                if (config.enableDither)
                {
                    ditherR.setAmplitude(currentBitDepthS);
                    double ditherNoiseS = ditherR.process(0.0, side);
                    side += ditherNoiseS;
                }

                quantizerR.setBitDepth(currentBitDepthS);
                side = quantizerR.process(side);

                // DECODE M/S → L/R
                stereo.decode(mid, side, left, right);
            }

            // Write back
            audio.samples[idxL] = left;
            audio.samples[idxR] = right;

            if (frame % (numFrames / 10) == 0)
            {
                std::cout << "." << std::flush;
            }
        }
    }

    std::cout << "\nDone!" << std::endl;

    // Save output
    std::cout << "Saving: " << outputFile << "..." << std::endl;
    if (AudioFile::save(outputFile, audio))
    {
        std::cout << "Success! File saved." << std::endl;
    }
    else
    {
        std::cerr << "Failed to save file!" << std::endl;
        return 1;
    }

    return 0;
}