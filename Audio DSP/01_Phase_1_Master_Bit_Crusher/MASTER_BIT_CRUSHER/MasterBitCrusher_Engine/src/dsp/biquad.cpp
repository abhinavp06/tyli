#include "biquad.h"
#include <cmath>
#include <numbers>

void Biquad::setCoefficients(double b0, double b1, double b2, double a1, double a2) {
    b0_ = b0;
    b1_ = b1;
    b2_ = b2;
    a1_ = a1;
    a2_ = a2;
}

void Biquad::reset() {
    x1_ = x2_ = 0.0;
    y1_ = y2_ = 0.0;
}

double Biquad::process(double x) {
    // The Difference Equation
    double y = b0_ * x + b1_ * x1_ + b2_ * x2_ - a1_ * y1_ - a2_ * y2_;

    // Shift the delay lines (bucket brigade)
    x2_ = x1_;
    x1_ = x;

    y2_ = y1_;
    y1_ = y;

    return y;
}

// RBJ Cookbook: Low-Pass Filter
void Biquad::makeLowPass(double sampleRate, double frequency, double q) {
    double omega = 2.0 * std::numbers::pi * frequency / sampleRate;
    double sinOmega = std::sin(omega);
    double cosOmega = std::cos(omega);
    double alpha = sinOmega / (2.0 * q);

    // Calculate coefficients
    double a0 = 1.0 + alpha;
    b0_ = ((1.0 - cosOmega) / 2.0) / a0;
    b1_ = (1.0 - cosOmega) / a0;
    b2_ = ((1.0 - cosOmega) / 2.0) / a0;
    a1_ = (-2.0 * cosOmega) / a0;
    a2_ = (1.0 - alpha) / a0;
}

// RBJ Cookbook: High-Pass Filter
void Biquad::makeHighPass(double sampleRate, double frequency, double q) {
    double omega = 2.0 * std::numbers::pi * frequency / sampleRate;
    double sinOmega = std::sin(omega);
    double cosOmega = std::cos(omega);
    double alpha = sinOmega / (2.0 * q);

    // Calculate coefficients
    double a0 = 1.0 + alpha;
    b0_ = ((1.0 + cosOmega) / 2.0) / a0;
    b1_ = -(1.0 + cosOmega) / a0;
    b2_ = ((1.0 + cosOmega) / 2.0) / a0;
    a1_ = (-2.0 * cosOmega) / a0;
    a2_ = (1.0 - alpha) / a0;
}