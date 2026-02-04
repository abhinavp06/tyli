#pragma once
#include <vector>

/**
 * @brief Cleanup and Coloring Filters
 * @source Merges logic from:
 * - 14_dc_blocker (DC Removal)
 * - 17_boxcar_filter (Anti-Aliasing)
 * - 13_leaky_integrator (Smoothing)
 * - 16_rbj_low_pass (Tone shaping)
 */
class FilterBank {
public:
    void init(double sampleRate);
    
    double processDCBlocker(double input);
    double processAntiAlias(double input); // Boxcar
    double processLowPass(double input);   // RBJ
    
private:
    // Filter states
    double prevX_ = 0.0, prevY_ = 0.0; 
};
