/*
 * A diagnostic tool to isolate Quantization Error.
 * It calculates "Input - Output". It plays only the error signal.
 * If a sine wave is bit crushed to 4 bits, we only want to hear the quantization noise.
 * Success Metric:
 *		Without dither - a buzzing, distorted tone is heard that changes pitch with the input (correlated error).
 *		With dither - A constant, steady white noise is heard (decorrelated error).
 * * Logic:
 * 1. Receive original Clean Input.
 * 2. Receive Processed (Crushed) Output.
 * 3. Calculate Difference (Residual).
 * 4. Apply "Microscope" gain (boost the error so we can hear/see it).
 */

#include <iostream>
#include <cmath>
#include <cstdint>
#include <algorithm>

enum class ListenMode {
    Normal,   // Standard Output
    Residual  // Null Test (Error Only)
};

class BitCrusher {
public:
    // --- User Parameters ---
    void set_mode(ListenMode mode) {
        current_mode = mode;
    }

    void update_parameters(float rate_divider, float bit_depth, float jitter_amount) {

        levels = std::pow(2.0f, bit_depth) - 1.0f;
        if (levels < 1.0f) levels = 1.0f;
        step_size = 1.0f / levels;


        jitter = jitter_amount;
        previous_block_size = std::max(1.0f, rate_divider);


        if (target_threshold < 1.0f) target_threshold = previous_block_size;
    }


    float process_sample(float input) {
        phase_count += 1.0f;


        if (phase_count >= target_threshold) {
            hold_value = input;
            phase_count = 0.0f;


            float jitter_offset = ((static_cast<float>(fast_random() * INVERTED_U_MAX)) - 0.5f) * jitter;
            target_threshold = previous_block_size + jitter_offset;

            if (target_threshold < 1.0f) target_threshold = 1.0f;
        }


        float processed_output = quantize_sample(hold_value) * step_size;

         // null tester (diagnostic logic)
        if (current_mode == ListenMode::Residual) {
            // Formula: (Processed - Input) * Microscope_Gain
            // 256.0f (+48dB) to make the bottom bits audible.
            return (processed_output - input) * 256.0f;
        }

        return processed_output;
    }

private:

    ListenMode current_mode = ListenMode::Normal;


    int silence_counter = 0;
    uint32_t state_1 = 12764, state_2 = 209375;
    const float INVERTED_U_MAX = 2.3283064365386963e-10f;


    float hold_value = 0.0f;
    float phase_count = 0.0f;
    float jitter = 0.0f;
    float target_threshold = 1.0f;
    float previous_block_size = 1.0f;


    float offset = 0.5f;
    float step_size = 0.0f;
    float levels = 1.0f;

    float fast_random() {
        state_1 = (state_1 * 1664525) + 1013904223;
        return static_cast<float>(state_1);
    }

    float add_dither(float input) {

        if (input > -1e-7f && input < 1e-7f) silence_counter++;
        else silence_counter = 0;


        if (silence_counter >= 512) return input;


        state_1 = state_1 * 1664525 + 1013904223;
        state_2 = state_2 * 1664525 + 1013904223;
        float r1 = (static_cast<float>(state_1) * INVERTED_U_MAX) - 0.5f;
        float r2 = (static_cast<float>(state_2) * INVERTED_U_MAX) - 0.5f;

        return input + ((r1 + r2) * step_size); // scale dither to 1 LSB
    }

    float quantize_sample(float input) {
        return std::floor(add_dither(input) * levels + offset);
    }
};