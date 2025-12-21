#include <iostream>
#include <vector>
#include <random>
#include <ctime>
#include <cstdlib>
#include <cmath>

class SurgicalQuantizer {
public:
	[[nodiscard]] float mid_tread_quanitzer(float input) const noexcept {
		return std::floor(input * levels + 0.5f) * inv_levels;
	}
	[[nodiscard]] float mid_rise_quantizer(float input) const noexcept {
		return (std::floor(input * levels) + 0.5f) * inv_levels;
	}
private:
	float levels{ 15.0f };  // assuming we had a bit rate of 4
	float inv_levels{ 1.0f / 15.0f };
};


int main() {
	// Assuming a float vector came in as the buffer.
	// Not going to actually implement the receival of a buffer.
	std::vector<float> hardware_buffer(BUFFER_SIZE);

	for (int i = 0; i < BUFFER_SIZE; i++) {
		hardware_buffer[i] = mid_rise_quantizer(hardware_buffer[i]); // for mid rise quantizer
		//hardware_buffer[i] = mid_tread_quantizer(hardware_buffer[i]); // for mid tread quantizer
	}

}