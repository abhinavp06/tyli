#include <iostream>
#include <vector>
#include <random>
#include <ctime>
#include <cstdlib>
#include <cmath>

class BitCrusherEngine {

public:
	void process_block(float* buffer, int buffer_size, bool use_mid_tread) {
		const float offset = use_mid_tread ? 0.5f : 0.0f;

		for (int i = 0; i < buffer_size; i++) {
			if (stretch_countdown <= 0) {
				float quantized_value = std::floor(buffer[i] * inverted_step_size + offset);
				if (!use_mid_tread) quantized_value += 0.5f;

				value_to_stretch = quantized_value * step_size;
				stretch_countdown = 10; // assuming block size is 10;
			}

			buffer[i] = value_to_stretch;

			stretch_countdown--;
		}
	}

private:
	float step_size{ 1.0f / 32768.0f };
	float inverted_step_size{ 32768.0f };
	float value_to_stretch = 0.0f;
	int stretch_countdown = 0;
};
