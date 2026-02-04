#pragma once
#include <cmath>
#include <algorithm>
#include <iostream>

/**
 * @brief The Core Bit Reduction Engine
 * @source Merges logic from:
 * - 03_surgical_quantizer (Mid-Tread/Rise logic)
 * - 02_architecture_based_bit_crusher (Bit depth calc)
 */
class Quantizer {
public:
    enum class Architecture { MidTread, MidRise };

    void setBitDepth(double bits);
    void setArchitecture(Architecture arch);
    
    // The "Crush" function
    double process(double input);

private:
    double currentBits_ = 16.0;
    double stepSize_ = 0.0;
    Architecture arch_ = Architecture::MidTread;
};
