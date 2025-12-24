/*
* Accept bit_depth and block_size (or rate_divider) as an input.
* Ensure all calculations are real-time safe (no new, no malloc, no std::pow) inside the sample loop.
* Pre-calculate the scales
* Use the roundf() function to pull the "fuzz" to the nearest bit-depth grid line.
*/

/*
*  **Requirements:**
*	1. Horizontal:
*		-> `rate_divider` with a `phase_accumulator`.
*		-> A `jitter` parameter that modulates the `phase_accumulator` threshold.
*	2. Gate: 
*		-> The 512-sample `silence_counter` with `epsilon` check.
*	3. Vertical:
*		-> The `TPDFDitherer` (2 LSB range, LCG-based).
*		-> Mid-Tread Quantization to the target `bit_depth`.
*	4. Optimization:
*		-> Pre-calculate the `inv_scale` so you aren't calling `pow()` or `1<<bit_depth` every sample.
* 
* NOTE: Dither after sample-and-hold. 
*		If we dither before, the noise gets "frozen" for multiple samples. 
*		This turns white noise into blocky, low-frequency noise which sounds like a chirping bird rather than a clean, analog-style hiss.
*/

#include <iostream>
#include <cmath>
#include <cstdint>

class BitCrusher {
public:
	void update_parameters(float block_size, float bit_depth, float jitter_amount) {
		previous_block_size = block_size;
		levels = std::pow(2.0f, bit_depth) - 1;
		step_size = 1.0f / levels;
		jitter = jitter_amount;
		previous_block_size = std::max(1.0f, block_size);
	}

	float add_dither(float input) {
		if (hold_value > -1e-7f && hold_value < 1e-7f) {
			silence_counter++;
		}
		else {
			silence_counter = 0;
		}

		float dither = 0.0f;

		if (silence_counter < 512) {
			state_1 = state_1 * 1664525 + 1013904223;
			state_2 = state_2 * 1664525 + 1013904223;

			float result_1 = (static_cast<float>(state_1) * INVERTED_U_MAX) - 0.5f;
			float result_2 = (static_cast<float>(state_2) * INVERTED_U_MAX) - 0.5f;
			dither = (result_1 + result_2) * step_size;
		}

		return input + dither;
	}

	float quantize_sample(float input) {
		return std::floor(add_dither(input) * levels + offset);
	}

	float process_sample(float input) {
		phase_count += 1.0f;

		float jitter_offset = ((static_cast<float>(fast_random() * INVERTED_U_MAX)) - 0.5f) * jitter;

		if (phase_count >= (previous_block_size + jitter_offset)) {
			hold_value = input;
			phase_count = 0.0f;
		}

		/*
		* Since we call this every frame, even when the captured sample hasn't changed,
		* the TPDF noise causes the output to "flicker' between the two nearest grid lines.
		* To the listener's ear, this flicking averages out to the smooth, original signal.
		*/
		return quantize_sample(hold_value) * step_size;

	}

private:
	// dither variables
	int silence_counter = 0;
	uint32_t state_1 = 12764, state_2 = 209375;
	const float F_MIN = -0.5f;
	const float F_MAX = 0.5f;
	const uint32_t U_MAX = std::numeric_limits<uint32_t>::max();
	const float INVERTED_U_MAX = 2.3283064365386963e-10f;
	// jitter variables
	float hold_value = 0.0f;
	float phase_count = 0.0f;
	float jitter = 0.0f;
	// common
	float previous_block_size = 0.0f;
	float offset = 0.5f; // assuming mid tread architecture
	float step_size = 0.0f;
	float levels = 0.0f;

	float fast_random() {
		// updating the state otherwise the jitter will be a fixed offset
		state_1 = (state_1 * 1664525) + 1013904223;
		return static_cast<float>(state_1);
	}
};