#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <iomanip>

// =============================================================================
// THE CLASS (The Sensor)
// =============================================================================
class EnvelopeFollower {
public:
    EnvelopeFollower() = default;

    void prepare(double sampleRate) {
        sampleRate_ = sampleRate;
        envelope_ = 0.0;
        calculateCoefficients();
    }

    void setAttackTime(double ms) {
        attackTimeMs_ = std::max(0.0, ms);
        calculateCoefficients();
    }

    void setReleaseTime(double ms) {
        releaseTimeMs_ = std::max(0.0, ms);
        calculateCoefficients();
    }

    // REAL-TIME SAFE: No malloc, no heavy math, branch optimized
    [[nodiscard]] double process(double input) noexcept {
        // 1. Rectify (Absolute Value)
        double absInput = std::abs(input);

        // 2. Ballistics Logic
        // If signal is rising (Input > Env), use Attack.
        // If signal is falling (Input <= Env), use Release.
        if (absInput > envelope_) {
            envelope_ += attackCoeff_ * (absInput - envelope_);
        }
        else {
            envelope_ += releaseCoeff_ * (absInput - envelope_);
        }

        // 3. Denormal Prevention (Safety for CPU)
        // Very small numbers can cause CPU spikes. Snap to zero.
        if (envelope_ < 1.0e-9) envelope_ = 0.0;

        return envelope_;
    }

    // Helper to peek at values without processing
    [[nodiscard]] double getCurrentValue() const { return envelope_; }

private:
    double sampleRate_ = 44100.0;
    double attackTimeMs_ = 10.0;  // Default 10ms
    double releaseTimeMs_ = 100.0; // Default 100ms

    double attackCoeff_ = 0.0;
    double releaseCoeff_ = 0.0;

    double envelope_ = 0.0; // The "Capacitor" (State Variable)

    void calculateCoefficients() {
        // Formula: coeff = 1 - exp( -1 / (time_sec * sample_rate) )
        // We convert ms to seconds by multiplying by 0.001

        if (attackTimeMs_ <= 0.0) {
            attackCoeff_ = 1.0; // Instant
        }
        else {
            attackCoeff_ = 1.0 - std::exp(-1.0 / (attackTimeMs_ * 0.001 * sampleRate_));
        }

        if (releaseTimeMs_ <= 0.0) {
            releaseCoeff_ = 1.0; // Instant
        }
        else {
            releaseCoeff_ = 1.0 - std::exp(-1.0 / (releaseTimeMs_ * 0.001 * sampleRate_));
        }
    }
};

// =============================================================================
// THE TEST HARNESS (The Verification)
// =============================================================================
int main() {
    // 1. Setup
    const double sampleRate = 44100.0;
    const double durationSeconds = 0.1; // Short burst for readable CSV
    const int totalSamples = static_cast<int>(sampleRate * durationSeconds);

    EnvelopeFollower follower;
    follower.prepare(sampleRate);

    // 2. Configure Ballistics
    // Fast Attack (catch the peak), Slow Release (smooth decay)
    follower.setAttackTime(1.0);   // 1ms
    follower.setReleaseTime(50.0); // 50ms

    std::cout << "Time(ms),Input,Envelope" << std::endl;
    std::cout << std::fixed << std::setprecision(6);

    // 3. Simulation Loop
    double phase = 0.0;
    double frequency = 100.0; // 100Hz Tone
    double twoPi = 6.28318530718;
    double phaseIncrement = (twoPi * frequency) / sampleRate;

    for (int n = 0; n < totalSamples; ++n) {
        // A. Generate Signal: Burst ON for first half, OFF for second half
        double input = 0.0;
        if (n < totalSamples / 2) {
            input = std::sin(phase);
            phase += phaseIncrement;
            if (phase >= twoPi) phase -= twoPi;
        }

        // B. Process
        double output = follower.process(input);

        // C. Log Data (Every 10th sample to keep it manageable)
        if (n % 10 == 0) {
            double timeMs = (n / sampleRate) * 1000.0;
            std::cout << timeMs << "," << input << "," << output << std::endl;
        }
    }

    return 0;
}