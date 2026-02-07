#include "oscillator.h"
#include <cmath>
#include <numbers>
#include <random>

void Oscillator::init(double sampleRate) {
    sampleRate_ = sampleRate;
    phase_ = 0.0;
    phaseInc_ = 0.0;
    wave_ = Waveform::Sine;
}

void Oscillator::setFrequency(double hz) {
    phaseInc_ = (2.0 * std::numbers::pi * hz) / sampleRate_;
}

void Oscillator::setWaveform(Waveform wave) {
    wave_ = wave;
}

double Oscillator::nextSample() {
    double output = std::sin(phase_);

    phase_ += phaseInc_;

    if (phase_ >= 2.0 * std::numbers::pi) {
        phase_ -= 2.0 * std::numbers::pi;
    }

    return output;
}