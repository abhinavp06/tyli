#pragma once
#include <cmath>

class EnobMeter {
public:
    void feed(double input, double output);
    double getResult(); // Returns Effective Number of Bits
    void reset();
private:
    double signalPower_ = 0.0;
    double noisePower_ = 0.0;
    size_t sampleCount_ = 0;
};
