/*
* In 02_Basic_Linear_Quantization, we created a 02_blocky_bit_rate_simulator.
* In this simulator, the dither was inefficient and very basic. It used rand() as well.
* This TPDF dither is much faster and efficient.
*/

/*
* Implementing a TPDF dither using the sum of 2 independent RPDF sources
*	1. The dither signal must be exactly [-1.0, 1.0] LSB peak-to-peak. No dividing the sum by 2.
*   2. Use a fast Linear Congruential Generator (LCG) or Xorshift.
*/

// FAST TPDF ENGINE

#include <iostream>
#include <cmath>

class TPDFDitherer {
public:
	float process_sample(float INV_SCALE) {
		state_1 = state_1 * 1664525 + 1013904223;
		state_2 = state_2 * 1664525 + 1013904223;
		/*
		* RESULT = F_MIN + ((F_MAX - F_MIN) * state_xyz * INVERTED_U_MAX);
		*/
		float result_1 = (static_cast<float>(state_1) * 2.3283064365386963e-10f) - 0.5f;
		float result_2 = (static_cast<float>(state_2) * 2.3283064365386963e-10f) - 0.5f;

		return (result_1 + result_2) * INV_SCALE;
	}

private:
	int bit_depth = 4;
	int silence_counter = 0;
	uint32_t state_1 = 12764, state_2 = 209375;
	const float F_MIN = -0.5f;
	const float F_MAX = 0.5f;
	const uint32_t U_MAX = std::numeric_limits<uint32_t>::max();
	const float INVERTED_U_MAX = 2.3283064365386963e-10f;
};