/*
* 1. Monitor input signal.
* 2. Use an epsilon (1e - 7) instead of hard 0.0f to detect silence.
* 3. If silence persists for 512 samples, the dither must drop to 0.0f.
* 4. The dither must "wake up" the very instant the signal crosses the threshold again.
*/

#include <iostream>
#include <cmath>

class TPDFDitherer {
public:
	float process_sample(float input, float inv_scale) {
		/*
		* DITHER GATE
		* Check if signal is below -140dB roughly (1e-7f)
		*/
		if(std::abs(input) < 1e-7f) {
			silence_counter++;
		}
		else {
			silence_counter = 0;
		}

		float dither = 0.0f;

		if (silence_counter < 512) { // 512 samples at 44.1kHz is about 11ms which is a standard "buffer-sized" window that feels natural.
			state_1 = state_1 * 1664525 + 1013904223;
			state_2 = state_2 * 1664525 + 1013904223;
			
			// RESULT = F_MIN + ((F_MAX - F_MIN) * state_xyz * INVERTED_U_MAX);
			float result_1 = (static_cast<float>(state_1) * 2.3283064365386963e-10f) - 0.5f;
			float result_2 = (static_cast<float>(state_2) * 2.3283064365386963e-10f) - 0.5f;

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
};