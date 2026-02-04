#!/bin/bash

# ==============================================================================
# MASTER BIT CRUSHER: COMPLETE ARCHITECTURE GENERATOR
# Phase 1: All Components
# ==============================================================================

PROJECT_ROOT="MasterBitCrusher_Engine"
echo ">>> Initializing Professional Audio Architecture: $PROJECT_ROOT"

# 1. Clean and Create Directory Hierarchy
rm -rf "$PROJECT_ROOT" # Clean start to avoid duplicates
mkdir -p "$PROJECT_ROOT"
cd "$PROJECT_ROOT"

mkdir -p src/dsp        # The Audio Processing Cores
mkdir -p src/analysis   # The Measurement Tools (Scopes, Meters)
mkdir -p src/utils      # File I/O and Helpers
mkdir -p src/app        # The CLI Application Logic
mkdir -p build          # Compiler Output
mkdir -p tests          # Audio assets

# ==============================================================================
# 2. DSP CORE STUBS (The Audio Path)
# ==============================================================================

# --- A. Quantizer (Amplitude Logic) ---
cat <<EOF > src/dsp/Quantizer.h
#pragma once
#include <cmath>
#include <algorithm>

/**
 * @brief The Core Bit Reduction Engine
 * @source Merges logic from:
 * - 03_surgical_quantizer (Mid-Tread/Rise logic)
 * - 02_architecture_based_bit_crusher (Bit depth calc)
 */
class Quantizer {
public:
    enum class Architecture { MidTread, MidRise };

    void setBitDepth(double bits);
    void setArchitecture(Architecture arch);
    
    // The "Crush" function
    double process(double input);

private:
    double currentBits_ = 16.0;
    double stepSize_ = 0.0;
    Architecture arch_ = Architecture::MidTread;
};
EOF

# --- B. Dither Engine (Spectral Cleanup) ---
cat <<EOF > src/dsp/Dither.h
#pragma once
#include <random>

/**
 * @brief Noise Generation and Shaping
 * @source Merges logic from:
 * - 01_tpdf_ditherer (Random generation)
 * - 02_dither_gate (Gating logic)
 * - 01_first_order_noise_shaper (Error feedback)
 */
class Dither {
public:
    void setAmplitude(double bits); // Auto-calcs based on 1 LSB
    void setGating(bool enabled);
    void setNoiseShaping(bool enabled);

    double process(double inputQuantized, double inputRaw);

private:
    double errorBuffer_ = 0.0; // For Noise Shaping
    // TODO: Add PRNG state
};
EOF

# --- C. Dynamics Processor (Envelope Ballistics) ---
cat <<EOF > src/dsp/Dynamics.h
#pragma once
#include <cmath>

/**
 * @brief Envelope Detection and Modulation
 * @source Merges logic from:
 * - 01_simple_envelope_follower (Ballistics)
 * - 02_dynamic_crusher (Logarithmic mapping)
 */
class EnvelopeFollower {
public:
    void setAttackRelease(double attMs, double relMs, double sampleRate);
    
    // Returns 0.0 (Silence) to 1.0 (Full Scale)
    double process(double input); 

private:
    double envelope_ = 0.0;
    double attackCoeff_ = 0.0;
    double releaseCoeff_ = 0.0;
};
EOF

# --- D. Resampler (Time Logic: Sample & Hold, Jitter) ---
cat <<EOF > src/dsp/Resampler.h
#pragma once

/**
 * @brief Variable Sample Rate Emulation & Jitter
 * @source Merges logic from:
 * - 07_sample_and_hold (Zero Order Hold)
 * - 02_blocky_bit_rate_simulator
 */
class Resampler {
public:
    void setRateDivisor(int div); // 1 = Normal, 10 = Lo-Fi
    void setJitterAmount(double amt); // Randomizes the Divisor (Clock Jitter)
    
    double process(double input);
    
private:
    int counter_ = 0;
    int currentLimit_ = 1;
    double holdValue_ = 0.0;
    double jitterAmount_ = 0.0;
};
EOF

# --- E. Biquad (Generic Filter Core) ---
cat <<EOF > src/dsp/Biquad.h
#pragma once

/**
 * @brief Generic IIR Filter Core (Direct Form I)
 * @source Merges logic from: 04_Biquad_Filter_Cascades, 16_RBJ_low_pass
 */
class Biquad {
public:
    void setCoefficients(double b0, double b1, double b2, double a1, double a2);
    
    // Pre-fab recipes
    void makeLowPass(double sampleRate, double frequency, double q);
    void makeHighPass(double sampleRate, double frequency, double q); // For DC Block
    
    double process(double input);

private:
    double x1_=0, x2_=0, y1_=0, y2_=0;
    double b0_=1, b1_=0, b2_=0, a1_=0, a2_=0;
};
EOF

# --- F. Filter Bank (Cleanup Managers) ---
cat <<EOF > src/dsp/FilterBank.h
#pragma once
#include "Biquad.h"

/**
 * @brief Cleanup and Coloring Filters
 * @source Merges logic from:
 * - 14_dc_blocker (DC Removal)
 * - 17_boxcar_filter (Anti-Aliasing)
 */
class FilterBank {
public:
    void init(double sampleRate);
    
    double processDCBlocker(double input);
    double processAntiAlias(double input); // Boxcar
    double processPostFilter(double input); // Optional Low Pass
    
private:
    Biquad dcBlocker_;
    // Boxcar state...
};
EOF

# --- G. Stereo Manager (Placeholder) ---
cat <<EOF > src/dsp/StereoManager.h
#pragma once

/**
 * @brief Phase 1, Section 3: Stereo Unlinking
 * @status PENDING THEORY
 */
class StereoManager {
public:
    // TODO: Implement Mid-Side encoding/decoding
    // TODO: Implement Dual-Mono unlinking
};
EOF

# ==============================================================================
# 3. ANALYSIS TOOLS (The Lab Equipment)
# ==============================================================================

# --- A. Null Tester ---
cat <<EOF > src/analysis/NullTester.h
#pragma once
#include <cmath>

class NullTester {
public:
    // Returns difference in dB
    static double compare(double original, double processed);
};
EOF

# --- B. ENOB Meter ---
cat <<EOF > src/analysis/EnobMeter.h
#pragma once
#include <cmath>

class EnobMeter {
public:
    void feed(double input, double output);
    double getResult(); // Returns Effective Number of Bits
};
EOF

# --- C. Linearity Scope ---
cat <<EOF > src/analysis/LinearityScope.h
#pragma once
#include <vector>
#include <string>

class LinearityScope {
public:
    // Captures the Input vs Output relationship
    void feed(double input, double output);
    void saveCSV(const std::string& filename);
private:
    struct DataPoint { double input; double output; };
    std::vector<DataPoint> buffer_;
};
EOF

# --- D. Glitch Detector ---
cat <<EOF > src/analysis/GlitchDetector.h
#pragma once

/**
 * @brief Discontinuity Detector
 * @source Merges logic from: 01_basic_glitch_simulator
 */
class GlitchDetector {
public:
    void setThreshold(double db); // e.g., -10dB jump
    bool check(double currentSample); // Returns TRUE if glitch detected
private:
    double prevSample_ = 0.0;
    double thresholdLinear_ = 0.1;
};
EOF

# ==============================================================================
# 4. UTILITIES & CONFIG
# ==============================================================================

# --- A. Audio File Handler ---
cat <<EOF > src/utils/AudioFile.h
#pragma once
#include <vector>
#include <string>

// Requires dr_wav.h and dr_flac.h implementation
struct AudioBuffer {
    std::vector<double> samples; // Interleaved
    int channels;
    int sampleRate;
};

class AudioFile {
public:
    static AudioBuffer load(const std::string& path);
    static bool save(const std::string& path, const AudioBuffer& buffer);
};
EOF

# --- B. Oscillator (Signal Generator) ---
cat <<EOF > src/utils/Oscillator.h
#pragma once

class Oscillator {
public:
    enum class Waveform { Sine, Triangle, Sawtooth, WhiteNoise };
    void init(double sampleRate);
    void setFrequency(double hz);
    void setWaveform(Waveform wave);
    double nextSample(); 
private:
    double phase_ = 0.0;
    double phaseInc_ = 0.0;
};
EOF

# --- C. CSV Writer ---
cat <<EOF > src/utils/CsvWriter.h
#pragma once
#include <string>
#include <vector>
#include <fstream>

class CsvWriter {
public:
    void open(const std::string& filename);
    void writeRow(const std::vector<double>& values);
    void close();
private:
    std::ofstream file_;
};
EOF

# --- D. Configuration Struct ---
cat <<EOF > src/app/Config.h
#pragma once

struct BitCrusherConfig {
    // Quantizer
    double bitDepth = 16.0;
    bool midTread = true;
    
    // Resampler
    int rateDivisor = 1;
    double jitterAmount = 0.0;
    
    // Dynamics
    double attackMs = 10.0;
    double releaseMs = 100.0;
    double modulationDepth = 0.0;
    
    // Dither
    bool ditherEnabled = true;
    bool noiseShaping = false;
};
EOF

# ==============================================================================
# 5. MAIN APPLICATION
# ==============================================================================

cat <<EOF > src/app/main.cpp
#include <iostream>
#include "Config.h"
#include "../dsp/Quantizer.h"
#include "../dsp/Dither.h"
#include "../dsp/Dynamics.h"
#include "../dsp/Resampler.h"
#include "../dsp/FilterBank.h"
#include "../utils/AudioFile.h"

int main() {
    std::cout << "--- MASTER BIT CRUSHER ENGINE v1.0 ---" << std::endl;
    std::cout << "1. Use AudioFile::load() to get samples." << std::endl;
    std::cout << "2. Iterate and apply DSP chain." << std::endl;
    std::cout << "3. Use AudioFile::save() to export." << std::endl;
    return 0;
}
EOF

# ==============================================================================
# 6. BUILD SYSTEM
# ==============================================================================

cat <<EOF > build.sh
#!/bin/bash
echo "Compiling Master Engine..."
# Compiles all cpp files in directories (Note: you must create the .cpp impl files!)
# For now, this just builds main.cpp to verify includes.
g++ -std=c++20 -I./src src/app/main.cpp -o build/crusher
echo "Done. Executable in build/crusher"
EOF
chmod +x build.sh

echo ">>> COMPLETE ARCHITECTURE GENERATED."
echo ">>> Location: $PROJECT_ROOT/"
echo ">>> Next Step: Start implementing the .cpp logic for src/utils/AudioFile.h"