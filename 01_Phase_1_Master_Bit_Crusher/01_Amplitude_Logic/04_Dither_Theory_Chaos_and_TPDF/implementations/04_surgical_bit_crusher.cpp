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
*/

#include <iostream>
#include <cmath>

class BitCrusher {
public:
	float process_sample(float input, float inv_scale) {

	}
	float add_dither(float input, float inv_scale) {
		if (std::abs(input) < 1e-7f) {
			silence_counter++;
		}
		else {
			silence_counter = 0;
		}

		float dither = 0.0f;

		if (silence_counter < 512) {
			state_1 = state_1 * 1664525 + 1013904223;
			state_2 = state_2 * 1664525 + 1013904223;

			float result_1 = (static_cast<float>(state_1) * 2.3283064365386963e-10f);
			float result_2 = (static_cast<float>(state_2) * 2.3283064365386963e-10f);
			dither = (result_1 + result_2) * inv_scale;
		}

		return input + dither;
	}

private:
	int silence_counter = 0;
	uint32_t state_1 = 12764, state_2 = 209375;
	const float F_MIN = -0.5f;
	const float F_MAX = 0.5f;
	const uint32_t U_MAX = std::numeric_limits<uint32_t>::max();
	const float INVERTED_U_MAX = 2.3283064365386963e-10f;
	float hold_value = 0.0f;
	int hold_counter = 0;
};