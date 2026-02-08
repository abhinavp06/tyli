#pragma once
#include <cmath>
#include <algorithm>

namespace DSP {

    class NoiseShaper {
    private:
        // State variable to hold the error from the previous sample
        float m_prev_error = 0.0f;

    public:
        NoiseShaper() = default;

        // Reset state (call this when playback starts/stops)
        void reset() {
            m_prev_error = 0.0f;
        }

        /**
         * Processes a single sample with 1st Order Noise Shaping.
         * * @param input_sample The high-res input (-1.0 to 1.0)
         * @param bit_depth Target bit depth (e.g., 8, 12, 16)
         * @return The noise-shaped, quantized sample
         */
        [[nodiscard]] float process(float input_sample, float bit_depth) {

            // 1. Calculate the number of steps
            // Using (2^N - 1) for mid-tread/standard quantization behavior
            const float steps = std::pow(2.0f, bit_depth) - 1.0f;

            // 2. ERROR FEEDBACK STEP:
            // Add the negative of the previous error to the current input.
            // We are "paying back the debt" of the previous rounding error.
            float compensated_input = input_sample - m_prev_error;

            // 3. Quantize the compensated input
            // Scale up -> Round -> Scale down
            float scaled = compensated_input * steps;
            float quantized_scaled = std::round(scaled);

            // (Optional: Clip to ensure we don't wrap around, 
            // though hard clipping adds its own harmonics)
            // quantized_scaled = std::clamp(quantized_scaled, -steps, steps);

            float output_sample = quantized_scaled / steps;

            // 4. Calculate the NEW error
            // Error = Actual Output - Input to the Quantizer
            // Note: We compare against 'compensated_input', not original 'input_sample'
            m_prev_error = output_sample - compensated_input;

            return output_sample;
        }
    };
}