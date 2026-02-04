#pragma once
#include <cmath>

class EnobMeter {
public:
    void feed(double input, double output);
    double getResult(); // Returns Effective Number of Bits
private:
    // TODO: Add SINAD calculation logic
};
