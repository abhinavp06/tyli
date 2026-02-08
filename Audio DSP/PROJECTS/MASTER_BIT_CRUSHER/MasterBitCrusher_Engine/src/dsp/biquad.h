#pragma once

/**
 * @brief Generic IIR Filter Core (Direct Form I)
 * @source Merges logic from:
 * - 04_Biquad_Filter_Cascades
 * - 16_RBJ_low_pass
 */
class Biquad {
public:
    void setCoefficients(double b0, double b1, double b2, double a1, double a2);

    // Pre-fab recipes
    void makeLowPass(double sampleRate, double frequency, double q);
    void makeHighPass(double sampleRate, double frequency, double q); // For DC Block

    double process(double input);
    void reset();

private:
    // State variables (z^-1, z^-2)
    double x1_ = 0, x2_ = 0;
    double y1_ = 0, y2_ = 0;

    // Coefficients
    double b0_ = 1, b1_ = 0, b2_ = 0, a1_ = 0, a2_ = 0;
};