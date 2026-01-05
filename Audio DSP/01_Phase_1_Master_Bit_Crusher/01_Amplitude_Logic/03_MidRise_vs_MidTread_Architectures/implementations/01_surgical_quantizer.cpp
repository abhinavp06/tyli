#include <iostream>
#include <vector>
#include <random>
#include <ctime>
#include <cstdlib>
#include <cmath>

class QuantizerEngine {

public:
	void process_block(float* buffer, int buffer_size, bool use_mid_tread) {
		/*
		* Mid tread ->
		*	In the case of a mid tread, we want to snap to the closest whole number.
		*	This is why an offset of 0.5f is required (since std::floor is a down only function)
		* Mid rise ->
		*	In the case of a mid rise, we want to avoid zero.
		*	This is why an offset is added after rounding off so that we never get a true 0.
		* 
		* Why not add an offset for mid rise anyways?
		* -> Because if we do that, we just have a mid tread which is up by 0.5 (assuming we still add 0.5f to the quantized value later)
		*/
		const float offset = use_mid_tread ? 0.5f : 0.0f;

		for (int i = 0; i < buffer_size; i++) {
			float quantized_value = std::floor(buffer[i] * inverted_step_size + offset);

			if (!use_mid_tread) quantized_value += 0.5f;

			buffer[i] = quantized_value * step_size;
		}
	}

private:
	float step_size{ 1.0f / 32768.0f }; // this is basically "stretching" the 0-1 range to 32768 steps - which gives the true value for a single step.
	float inverted_step_size{ 32768.0f };
};
