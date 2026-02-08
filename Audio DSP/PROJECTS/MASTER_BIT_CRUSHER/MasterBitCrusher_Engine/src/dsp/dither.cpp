#include "dither.h"
#include <cmath>
#include <random>

void Dither::setAmplitude(double bits) {
    double levels = std::pow(2.0, bits) - 1.0;
    double stepSize = 2.0 / levels;

    amplitude_ = stepSize;
}

void Dither::setGating(bool enabled) {
    gatingEnabled_ = enabled;
}

void Dither::setNoiseShaping(bool enabled) {
    noiseShaperEnabled_ = enabled;
}

double Dither::process(double inputQuantized, double inputRaw) {
    if (gatingEnabled_) {
        if (std::abs(inputRaw) < gateThreshold_) {
            silenceCounter_++;
        }
        else {
            silenceCounter_ = 0;
        }

        if (silenceCounter_ >= SILENCE_LIMIT) {
            return 0.0;
        }
    }

    static std::mt19937 gen(12345);
    static std::uniform_real_distribution<double> dist(0.0, 1.0);

    double rand1 = dist(gen);
    double rand2 = dist(gen);

    double tpdf = (rand1 + rand2 - 1.0) * amplitude_;

    if (noiseShaperEnabled_) {
        double error = inputQuantized - inputRaw;
        tpdf += errorBuffer_ * 0.5;
        errorBuffer_ = error;
    }

    return tpdf;
}

void Dither::reset() {
    errorBuffer_ = 0.0;
}