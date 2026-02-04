#pragma once

/**
 * @brief Variable Sample Rate Emulation
 * @source Merges logic from:
 * - 07_sample_and_hold (Zero Order Hold)
 * - 02_blocky_bit_rate_simulator
 */
class Resampler {
public:
    void setRateDivisor(int div); // 1 = Normal, 10 = Lo-Fi
    void setJitterAmount(double amt); // Randomizes the Divisor
    double process(double input) {
        // Logic:
        // 1. Check if counter >= currentLimit
        // 2. If yes, update output and randomize next limit (Jitter)
        // 3. Return held output
    }
    
private:
    int counter_ = 0;
    double holdValue_ = 0.0;
};
