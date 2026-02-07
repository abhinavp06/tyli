#include "dynamics.h"
#include <cmath>
#include <algorithm>

void EnvelopeFollower::setAttackRelease(double attMs, double relMs, double sampleRate) {
    sampleRate_ = sampleRate;
    attackTimeMs_ = attMs;
    releaseTimeMs_ = relMs;

    // Calculate coefficients
    // Formula: coeff = 1 - exp(-1 / (time_sec * sample_rate))
    if (attackTimeMs_ <= 0.0) {
        attackCoeff_ = 1.0; // Instant
    }
    else {
        attackCoeff_ = 1.0 - std::exp(-1.0 / (attackTimeMs_ * 0.001 * sampleRate_));
    }

    if (releaseTimeMs_ <= 0.0) {
        releaseCoeff_ = 1.0; // Instant
    }
    else {
        releaseCoeff_ = 1.0 - std::exp(-1.0 / (releaseTimeMs_ * 0.001 * sampleRate_));
    }
}

double EnvelopeFollower::process(double input) {
    double absInput = std::abs(input);

    if (absInput > envelope_) {
        envelope_ += attackCoeff_ * (absInput - envelope_);
    }
    else {
        envelope_ += releaseCoeff_ * (absInput - envelope_);
    }

    if (envelope_ < 1.0e-9) {
        envelope_ = 0.0;
    }

    return envelope_;
}

void EnvelopeFollower::reset() {
    envelope_ = 0.0;
}