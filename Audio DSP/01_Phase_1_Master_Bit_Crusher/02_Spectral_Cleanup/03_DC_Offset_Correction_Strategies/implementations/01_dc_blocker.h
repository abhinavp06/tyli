#pragma once
#include <cmath>
#include <numbers>

class DCBlocker {
public:
    DCBlocker() = default;

    // Initialize with sample rate to calculate R correctly
    void init(double sampleRate, double cutoffFreq = 20.0) {
        // Calculate R based on desired cutoff frequency
        // R = exp(-2pi * fc / fs)
        // Ideally R should be between 0.995 and 0.999 for audio

        m_R = std::exp(-2.0 * std::numbers::pi * cutoffFreq / sampleRate);
        reset();
    }

    void reset() {
        m_prevX = 0.0;
        m_prevY = 0.0;
    }

    // The Process Loop: No allocations, purely arithmetic
    [[nodiscard]] double process(double x) noexcept {
        // y[n] = x[n] - x[n-1] + R * y[n-1]
        double y = x - m_prevX + (m_R * m_prevY);

        // Update state
        m_prevX = x;
        m_prevY = y;

        return y;
    }

private:
    double m_R = 0.995; // Default coefficient
    double m_prevX = 0.0;
    double m_prevY = 0.0;
};