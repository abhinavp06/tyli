#pragma once
#include <cmath>
#include <algorithm> // For std::clamp

namespace DSP {

    /**
     * @brief A First-Order IIR Low Pass Filter (Leaky Integrator).
     * * Used for smoothing control values (zipper noise reduction)
     * and basic tone shaping.
     */
    class LeakyIntegrator {
    private:
        // STATE VARIABLE
        // This is the "Memory" of the filter. It holds y[n-1].
        float m_output = 0.0f;

        // COEFFICIENT
        // Determines the speed of the filter.
        // 1.0 = Instant (Pass-through)
        // 0.001 = Very Slow (Heavy filtering)
        float m_alpha = 1.0f;

    public:
        LeakyIntegrator() = default;

        /**
         * @brief Sets the smoothing factor directly.
         * @param alpha Range [0.0, 1.0].
         * Higher = Faster response (Less filtering).
         * Lower = Slower response (More filtering).
         */
        void setAlpha(float alpha) {
            // Safety: Clamp to valid range to prevent explosions
            m_alpha = std::clamp(alpha, 0.0f, 1.0f);
        }

        /**
         * @brief Helper to set alpha based on Sample Rate and desired Cutoff Hz.
         * Note: This is an approximation valid for low frequencies relative to Nyquist.
         */
        void setCutoff(float sampleRate, float cutoffHz) {
            float dt = 1.0f / sampleRate;
            float rc = 1.0f / (2.0f * 3.14159265f * cutoffHz);
            // Derive alpha from RC circuit time constant
            float alpha = dt / (dt + rc);
            setAlpha(alpha);
        }

        /**
         * @brief Resets the filter state.
         * Call this when audio playback starts to avoid old values "leaking" in.
         */
        void reset(float value = 0.0f) {
            m_output = value;
        }

        /**
         * @brief Processes a single sample.
         * Formula: y[n] = y[n-1] + alpha * (x[n] - y[n-1])
         */
        [[nodiscard]] float process(float input) {
            // 1. Calculate the difference between where we are and where we want to be.
            float delta = input - m_output;

            // 2. Move a fraction (alpha) of that distance.
            m_output = m_output + (m_alpha * delta);

            // 3. DENORMAL PROTECTION (CRITICAL)
            // If the value is infinitesimally small (but not zero),
            // the CPU takes a massive performance hit.
            // We snap it to true zero.
            if (std::abs(m_output) < 1.0e-15f) {
                m_output = 0.0f;
            }

            return m_output;
        }
    };
}