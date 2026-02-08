#pragma once
#include <random>
#include <cmath>

/**
 * @brief Noise Generation and Shaping
 * @source Merges logic from:
 * - 01_tpdf_ditherer (Random generation)
 * - 02_dither_gate (Gating logic)
 * - 01_first_order_noise_shaper (Error feedback)
 */
class Dither
{
public:
    void setSeed(unsigned int seed);
    void setAmplitude(double bits);
    void setGating(bool enabled);
    void setNoiseShaping(bool enabled);
    void setGateThreshold(double threshold);

    double process(double inputQuantized, double inputRaw);

    void reset();

private:
    double amplitude_ = 0.0;
    double errorBuffer_ = 0.0;
    bool gatingEnabled_ = false;
    bool noiseShaperEnabled_ = false;
    double gateThreshold_ = 1e-7; // ~140dB
    int silenceCounter_ = 0;
    static const int SILENCE_LIMIT = 512; // ~11ms at 44.1kHz
    // Random generator (per-instance)
    std::mt19937 gen_{12345};
};
