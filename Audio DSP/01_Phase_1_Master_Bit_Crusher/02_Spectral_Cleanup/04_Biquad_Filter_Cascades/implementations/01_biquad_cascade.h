#pragma once
#include <vector>
#include <cmath>
#include <numbers>

struct BiquadCoeffs {
    double b0 = 1.0, b1 = 0.0, b2 = 0.0;
    double a1 = 0.0, a2 = 0.0;
};

class Biquad {
public:
    Biquad() = default;

    void setCoeffs(const BiquadCoeffs& coeffs) {
        m_coeffs = coeffs;
    }

    void reset() {
        m_x1 = m_x2 = 0.0;
        m_y1 = m_y2 = 0.0;
    }

    // The Engine: Direct Form I
    [[nodiscard]] double process(double x) noexcept {
        // y[n] = b0*x[n] + b1*x[n-1] + b2*x[n-2] - a1*y[n-1] - a2*y[n-2]

        double y = m_coeffs.b0 * x
            + m_coeffs.b1 * m_x1
            + m_coeffs.b2 * m_x2
            - m_coeffs.a1 * m_y1
            - m_coeffs.a2 * m_y2;

        // Shift the delay lines (bucket brigade)
        // Values move from "1 step back" to "2 steps back"
        m_x2 = m_x1;
        m_x1 = x;

        m_y2 = m_y1;
        m_y1 = y;

        return y;
    }

private:
    BiquadCoeffs m_coeffs;

    // State registers (memory)
    double m_x1 = 0.0; // x[n-1]
    double m_x2 = 0.0; // x[n-2]
    double m_y1 = 0.0; // y[n-1]
    double m_y2 = 0.0; // y[n-2]
};

// The Container for Cascading
class BiquadCascade {
public:
    void addBiquad(const BiquadCoeffs& coeffs) {
        Biquad bq;
        bq.setCoeffs(coeffs);
        m_stages.push_back(bq);
    }

    void reset() {
        for (auto& stage : m_stages) {
            stage.reset();
        }
    }

    [[nodiscard]] double process(double x) noexcept {
        double temp = x;
        // Daisy chain: Output of one is input of next
        for (auto& stage : m_stages) {
            temp = stage.process(temp);
        }
        return temp;
    }

private:
    std::vector<Biquad> m_stages;
};