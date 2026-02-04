#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <iomanip>

// =============================================================================
// 1. THE SENSOR (Envelope Follower)
// =============================================================================
class EnvelopeFollower {
public:
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

    [[nodiscard]] double process(double input) noexcept {
        double absInput = std::abs(input);

        if (absInput > envelope_) {
            envelope_ += attackCoeff_ * (absInput - envelope_);
        }
        else {
            envelope_ += releaseCoeff_ * (absInput - envelope_);
        }

        if (envelope_ < 1.0e-9) envelope_ = 0.0;
        return envelope_;
    }

private:
    double sampleRate_ = 44100.0;
    double attackTimeMs_ = 10.0;
    double releaseTimeMs_ = 100.0;
    double attackCoeff_ = 0.0;
    double releaseCoeff_ = 0.0;
    double envelope_ = 0.0;

    void calculateCoefficients() {
        if (attackTimeMs_ <= 0.0) attackCoeff_ = 1.0;
        else attackCoeff_ = 1.0 - std::exp(-1.0 / (attackTimeMs_ * 0.001 * sampleRate_));

        if (releaseTimeMs_ <= 0.0) releaseCoeff_ = 1.0;
        else releaseCoeff_ = 1.0 - std::exp(-1.0 / (releaseTimeMs_ * 0.001 * sampleRate_));
    }
};

// =============================================================================
// 2. THE PROCESSOR (Dynamic Bit Crusher)
// =============================================================================
class DynamicBitCrusher {
public:
    enum class Mode {
        Linear,
        Logarithmic
    };

    void prepare(double sampleRate) {
        sampleRate_ = sampleRate;
        follower.prepare(sampleRate);

        // Tuned Ballistics
        follower.setAttackTime(5.0);
        follower.setReleaseTime(200.0);
    }

    // Configuration
    void setBaseBits(double bits) { baseBits_ = bits; }
    void setModulationDepth(double depth) { modDepth_ = depth; }
    void setMode(Mode mode) { currentMode_ = mode; }

    struct Result {
        double processedSample;
        double currentEnvelope;
        double currentBits;
    };

    [[nodiscard]] Result process(double input) {
        // A. CONTROL PATH (The Brain)
        double env = follower.process(input);
        double targetBits = 0.0;

        // B. MODULATION LOGIC (The Switch)
        if (currentMode_ == Mode::Linear) {
            targetBits = calculateBitsLinear(env);
        }
        else {
            targetBits = calculateBitsLogarithmic(env);
        }

        // C. AUDIO PATH (The Muscle)
        double stepSize = 2.0 / std::pow(2.0, targetBits);

        // Mid-Tread Quantization
        double tmp = input / stepSize;
        tmp = std::round(tmp);
        double output = tmp * stepSize;

        return { output, env, targetBits };
    }

private:
    EnvelopeFollower follower;
    double baseBits_ = 4.0;
    double modDepth_ = 12.0;
    Mode currentMode_ = Mode::Linear;

    // --- LINEAR MAPPING ---
    // Direct correlation: 50% volume = 50% modulation
    [[nodiscard]] double calculateBitsLinear(double env) const {
        double rawBits = baseBits_ + (env * modDepth_);
        return std::clamp(rawBits, 1.0, 16.0);
    }

    // --- LOGARITHMIC MAPPING ---
    // Decibel correlation: Matches human hearing.
    // Stays clean longer, crushes only at very low volumes.
    [[nodiscard]] double calculateBitsLogarithmic(double env) const {
        // 1. Convert to dB (Safe log)
        double db = 20.0 * std::log10(env + 1.0e-9);

        // 2. Normalize based on Sensitivity Floor (-60dB)
        const double noiseFloorDb = -60.0;

        // Map -60dB..0dB to 0.0..1.0
        double logEnv = (db - noiseFloorDb) / (0.0 - noiseFloorDb);

        // 3. Clamp normalized range (0.0 to 1.0)
        logEnv = std::clamp(logEnv, 0.0, 1.0);

        // 4. Calculate final bits
        double rawBits = baseBits_ + (logEnv * modDepth_);
        return std::clamp(rawBits, 1.0, 16.0);
    }
};

// =============================================================================
// 3. THE SIMULATION
// =============================================================================
int main() {
    const double sampleRate = 44100.0;
    const double duration = 2.0;
    const int totalSamples = static_cast<int>(sampleRate * duration);

    DynamicBitCrusher crusher;
    crusher.prepare(sampleRate);
    crusher.setBaseBits(4.0);
    crusher.setModulationDepth(12.0);

    // *** TOGGLE THIS TO COMPARE ***
    // DynamicBitCrusher::Mode::Linear 
    // DynamicBitCrusher::Mode::Logarithmic
    crusher.setMode(DynamicBitCrusher::Mode::Linear);

    std::cout << "Time(s),Input,Envelope,Bits,Output" << std::endl;
    std::cout << std::fixed << std::setprecision(6);

    double phase = 0.0;
    double frequency = 220.0;
    double twoPi = 6.28318530718;
    double phaseInc = (twoPi * frequency) / sampleRate;

    for (int n = 0; n < totalSamples; ++n) {
        // Linear fade out: 1.0 -> 0.0
        double amplitude = 1.0 - (double(n) / totalSamples);
        double input = amplitude * std::sin(phase);

        phase += phaseInc;
        if (phase >= twoPi) phase -= twoPi;

        auto result = crusher.process(input);

        if (n % 500 == 0) {
            double time = n / sampleRate;
            std::cout << time << ","
                << input << ","
                << result.currentEnvelope << ","
                << result.currentBits << ","
                << result.processedSample << std::endl;
        }
    }

    return 0;
}