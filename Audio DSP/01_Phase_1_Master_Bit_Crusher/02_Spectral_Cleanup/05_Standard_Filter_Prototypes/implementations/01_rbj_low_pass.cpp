#include <cmath>
#include <cstdint>

enum class FilterPrototype { Butterworth, ChebyshevTypeI, ChebyshevTypeII, Elliptic, Bessel };

struct RbjFilter {
    double b0 = 1.0, b1 = 0.0, b2 = 0.0;
    double a1 = 0.0, a2 = 0.0;
    double x1 = 0.0, x2 = 0.0;  // State (memory)
    double y1 = 0.0, y2 = 0.0;

    void setCoefficients(double fs, double fc, double Q, double rippleDb, FilterPrototype proto) {
        double omega = 2.0 * M_PI * fc / fs;
        double sinw = std::sin(omega);
        double cosw = std::cos(omega);
        double alpha = sinw / (2.0 * Q);
        double beta = std::sqrt(1.0 / (std::pow(10.0, rippleDb / 40.0)) - 1.0);  // For Chebyshev ripple

        // RBJ Cookbook formulas, adapted per prototype (simplified for low-pass)
        switch (proto) {
        case FilterPrototype::Butterworth:  // Flat passband, Q=0.707 implicit for 2nd order
            b0 = (1.0 - cosw) / 2.0;
            b1 = 1.0 - cosw;
            b2 = (1.0 - cosw) / 2.0;
            a1 = -2.0 * cosw;
            a2 = 1.0 - sinw / std::sqrt(2.0);  // Adjust for max flat
            break;
        case FilterPrototype::ChebyshevTypeI:  // Passband ripple
            b0 = (1.0 - cosw) / 2.0 * (1.0 + beta);
            b1 = 1.0 - cosw;
            b2 = b0;
            a1 = -2.0 * cosw / (1.0 + beta);
            a2 = (1.0 - beta) / (1.0 + beta);
            break;
            // Add TypeII (stopband ripple), Elliptic (min order), Bessel (linear phase) similarly via RBJ eqs.
        default: /* Handle others */ break;
        }
        // Normalize b coeffs by a0 (implicit 1.0 in direct form)
    }

    float process(float x) noexcept {
        float y = static_cast<float>(b0 * x + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2);
        x2 = x1; x1 = x;
        y2 = y1; y1 = y;
        return y;
    }
};

// Usage in bit crusher process loop:
// RbjFilter filter;
// filter.setCoefficients(44100.0, 5000.0, 0.707, 0.5, FilterPrototype::Butterworth);
// output = filter.process(crushedSample);