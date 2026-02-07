#pragma once
#include <cmath>
#include <algorithm>

/**
 * @brief Envelope Detection and Modulation
 * @source Merges logic from:
 * - 01_simple_envelope_follower (Ballistics)
 * - 02_dynamic_crusher (Logarithmic mapping)
 */
class EnvelopeFollower {
public:
    void setAttackRelease(double attMs, double relMs, double sampleRate);
    double process(double input); // Returns 0.0 - 1.0
    void reset();

private:
    double sampleRate_ = 44100.0;
    double attackTimeMs_ = 5.0;
    double releaseTimeMs_ = 200.0;
    double attackCoeff_ = 0.0;
    double releaseCoeff_ = 0.0;
    double envelope_ = 0.0;
};
