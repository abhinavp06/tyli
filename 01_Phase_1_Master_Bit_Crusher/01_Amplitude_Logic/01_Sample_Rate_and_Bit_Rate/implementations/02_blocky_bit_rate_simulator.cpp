#include <iostream>
#include <vector>
#include <random>
#include <ctime>
#include <cstdlib>
#include <cmath>

/*
* STEPS =>
* 1. The first thing to do is imagine a 128 buffer sized block. This is not a "full wave". It is just a part of a wave. 128 parts. That's it. It's a simple graph which plots points.
* 2. The block size determines how many vertical "strips" I am cutting off (n-1). The value at the 0th index is cloned all the way till the n-1 index.
* 3. Now since we are simply stretching this value, we actually will end up bringing in more variation in some parts of the audio since the amplitude of the N+1th sample might have changed drastically (depending on the bit depth). This difference in combination with the "stretched out value of the intiial sample (the held value)" is what creates the block (imagine a corner of a square).
* 4. The jitter's job is to make the "hold" durations irregular so that it can sound more "natural" after bit crushing.
* 5. The dither's job is to actually help the speaker not go crazy by protecting the amount of "swing" between two amplitude levels.
*       i. This happens over multiple buffer cycles. Imagining it with a single buffer sized sample will make it difficult to understand.
*		ii. Dither does not smooth the wave in the time domain; it decorrelates the error. Instead of the speaker making a predictable buzzing mistake it makes a random hissing mistake. The detail comes from the fact that the average of that hiss is the original signal.
* Note: 
* The reason bit crusher sounds like a half-completed version of a dubstep bass.
* The fuzziness/muddiness is due to information loss caused by the stretching
* The "sharper" sound is due to the "huge" differences between the amplitudes of the held value and the n+1th sample value (since the speaker suddenly needs to account for this amplitude change which causes the speaker cone to create a big burst of high frequency energy)
*/

const int BLOCK_SIZE = 10;
const int BUFFER_SIZE = 128; // This is a constant simply to act as a guardrail incase there is a hardware malfunction. Ideally the hardware will already decide the buffer size and the program will be written FOR that hardware.
const int BIT_DEPTH = 4; // Keeping this low to see an actual impact. If this was very high (like 16), the amplitude difference won't be much.
const float Y_AXIS_LEVEL_COUNT = std::pow(2.0f, BIT_DEPTH - 1.0f); // The dither needs to have normalized rungs based on the bit depth

float value_to_stretch = 0.0f;
int stretch_countdown = 0;

int get_jitter() {
	int result = rand() % (BLOCK_SIZE + 1);

	if (result == 0) return 1;
	return result;
}

void process_buffer(std::vector<float>& buffer) {
	for (int i = 0; i < BUFFER_SIZE; i++) {
		if (stretch_countdown <= 0) {
			value_to_stretch = buffer[i];
			stretch_countdown = get_jitter();
		}

		stretch_countdown--;

		// Adding dither
		float shaken_value = (value_to_stretch + ((float)rand() / (float)RAND_MAX - 0.5f) * (1.0f / Y_AXIS_LEVEL_COUNT));
		float dithered_value =  std::round(shaken_value * Y_AXIS_LEVEL_COUNT) / Y_AXIS_LEVEL_COUNT;
		buffer[i] = dithered_value;
	}
}

int main() {
	// Assuming a float vector came in as the buffer.
	// Not going to actually implement the receival of a buffer.
	std::vector<float> hardware_buffer(BUFFER_SIZE);

	process_buffer(hardware_buffer);

}