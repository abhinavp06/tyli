#pragma once
#include <cmath>
#include <iostream>

/**
 * @brief Discontinuity Detector
 * @source Merges logic from:
 * - 01_basic_glitch_simulator
 */
class GlitchDetector {
public:
    void setThreshold(double db); // e.g., -10dB jump

    // Returns TRUE if a massive jump is detected
    bool check(double currentSample);

private:
    double prevSample_ = 0.0;
    double thresholdLinear_ = 0.1;
};