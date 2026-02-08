#pragma once

/**
 * @brief Variable Sample Rate Emulation
 * @source Merges logic from:
 * - 07_sample_and_hold (Zero Order Hold)
 * - 02_blocky_bit_rate_simulator
 */
class Resampler {
public:
    enum class Mode { Naive, Boxcar };
    void setMode(Mode mode);
    void setRateDivisor(int div); // 1 = Normal, 10 = Lo-Fi
    void setJitterAmount(double amt); // Randomizes the Divisor
    double process(double input);
    
private:
    int counter_ = 0;
    double holdValue_ = 0.0;
    Mode mode_ = Mode::Boxcar; // default to anti-aliased
    int rateDivisor_ = 1;     // Default to 1 (no reduction)
    int currentLimit_ = 1;    // The current target count
    double jitterAmount_ = 0.0; // 0.0 to 1.0
    double accumulator_ = 0.0;
    double randomFloat();
};
