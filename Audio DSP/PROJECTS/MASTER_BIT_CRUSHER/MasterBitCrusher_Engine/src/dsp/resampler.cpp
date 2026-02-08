#include "resampler.h"
#include <random>
#include <algorithm>

void Resampler::setRateDivisor(int div) {
    if (div < 1) div = 1;
    rateDivisor_ = div;
    currentLimit_ = div;
}

void Resampler::setJitterAmount(double amt) {
    jitterAmount_ = std::clamp(amt, 0.0, 1.0);
}

void Resampler::setMode(Mode mode) {
    mode_ = mode;
}

double Resampler::process(double input) {
    if (mode_ == Mode::Naive) {
        counter_++;
        if (counter_ >= currentLimit_) {
            holdValue_ = input;
            counter_ = 0;
            double jitterOffset = (randomFloat() - 0.5) * 2.0 * jitterAmount_;
            currentLimit_ = rateDivisor_ + static_cast<int>(jitterOffset * rateDivisor_);
            if (currentLimit_ < 1) currentLimit_ = 1;
        }
        return holdValue_;
    }

    else {
        accumulator_ += input;
        counter_++;

        if (counter_ >= currentLimit_) {
            holdValue_ = accumulator_ / static_cast<double>(counter_);

            accumulator_ = 0.0;
            counter_ = 0;

            double jitterOffset = (randomFloat() - 0.5) * 2.0 * jitterAmount_;
            currentLimit_ = rateDivisor_ + static_cast<int>(jitterOffset * rateDivisor_);
            if (currentLimit_ < 1) currentLimit_ = 1;
        }

        return holdValue_;
    }
}

double Resampler::randomFloat() {
    static std::mt19937 gen(12345);
    static std::uniform_real_distribution<double> dist(0.0, 1.0);
    return dist(gen);
}