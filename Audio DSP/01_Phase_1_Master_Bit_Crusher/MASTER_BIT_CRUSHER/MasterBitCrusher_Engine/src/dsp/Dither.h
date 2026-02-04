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
class Dither {
public:
    void setAmplitude(double bits); // Auto-calcs based on 1 LSB
    void setGating(bool enabled);
    void setNoiseShaping(bool enabled);

    double process(double inputQuantized, double inputRaw);

private:
    double errorBuffer_ = 0.0; // For Noise Shaping
    // TODO: Add PRNG state
};
