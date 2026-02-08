#include "quantizer.h"
#include <cmath>
#include <algorithm>

void Quantizer::setBitDepth(double bits) {
    currentBits_ = std::clamp(bits, 1.0, 32.0);

    double levels = std::pow(2.0, currentBits_) - 1.0;
    stepSize_ = 2.0 / levels;
}

void Quantizer::setArchitecture(Architecture arch) {
    arch_ = arch;
}

double Quantizer::process(double input) {
    input = std::clamp(input, -1.0, 1.0);

    if (arch_ == Architecture::MidTread) {
        double scaled = input / stepSize_;
        double quantized = std::round(scaled);
        return quantized * stepSize_;
    }
    else {
        double scaled = input / stepSize_;
        double quantized = std::floor(scaled) + 0.5;
        return quantized * stepSize_;
    }
}