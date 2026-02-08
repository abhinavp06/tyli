#include "enob_meter.h"
#include <cmath>
#include <algorithm>

void EnobMeter::feed(double input, double output) {
    signalPower_ += input * input;

    double error = output - input;
    noisePower_ += error * error;

    sampleCount_++;
}

double EnobMeter::getResult() {
    if (sampleCount_ == 0) return 0.0;

    double rmsSignal = std::sqrt(signalPower_ / sampleCount_);
    double rmsNoise = std::sqrt(noisePower_ / sampleCount_);

    if (rmsNoise < 1e-12) return 32.0;

    double sinad_dB = 20.0 * std::log10(rmsSignal / rmsNoise);

    double enob = (sinad_dB - 1.76) / 6.02;

    return std::clamp(enob, 0.0, 32.0);
}

void EnobMeter::reset() {
    signalPower_ = 0.0;
    noisePower_ = 0.0;
    sampleCount_ = 0;
}