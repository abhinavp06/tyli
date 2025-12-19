#include <iostream>
#include <vector>
#include <deque>
#include <cmath>
#include <chrono>
#include <thread>
#include <algorithm>

const int TOTAL_ROWS = 30;
const int ZERO_AUDIO_ROW = 15;
const int TOP_ROW = 0;
const int BOTTOM_ROW = TOTAL_ROWS - 1;
const int GLITCH_THRESHOLD = 2;

const std::string RED = "\033[1;31m";
const std::string GREEN = "\033[1;32m";
const std::string BLUE = "\033[1;34m";
const std::string RESET = "\033[0m";

int coordinate_mapper(float value) {
	/*
	* Even though we have clamping at the bottom of the function, this is still important.
	* This ensures that we clip before anything is processed. This doesn't cause any weird math issues.
	*/
	if (value > 1.0f) value = 1.0f;
	if (value < -1.0f) value = -1.0f;

	int result = std::round((1.0f - value) * (TOTAL_ROWS / 2));

	/*
	* This is needed because if TOTAL_ROWS is 20, result will have a value of 20.
	* This will cause an array out of bounds error.
	*/
	if (TOTAL_ROWS % 2 == 0) result -= 1;

	// Clamping
	if (result > BOTTOM_ROW) return BOTTOM_ROW;
	if (result < TOP_ROW) return TOP_ROW;

	return result;
}

void render_wave(const std::vector<float>& buffer) {
	std::vector<std::string> canvas(TOTAL_ROWS, std::string(buffer.size(), ' '));
	canvas[ZERO_AUDIO_ROW] = std::string(buffer.size(), '-');

	int previous_row = -1;

	for (int i = 0; i < buffer.size(); i++) {
		int target_row = coordinate_mapper(buffer[i]);

		/*
		* buffer[0] means the 0th column
		* the coordinate_mapper decides which row the star should be printed in
		* we are basically printing TOTAL_ROWS strings. 
		* The canvas is prefilled before being displayed on the terminal.
		*/
		if (previous_row != -1) {
			int distance = std::abs(target_row - previous_row);
			if (distance > GLITCH_THRESHOLD) {
				int start = std::min(previous_row, target_row);
				int end = std::max(previous_row, target_row);
				for (int r = start + 1; r < end; r++) {
					canvas[r][i] = '|';
				}
			}
		}

		canvas[target_row][i] = '*';
		previous_row = target_row;

	}

	std::cout << "--- IEM DSP VISUALIZER ---" << std::endl;
	for (const std::string& row : canvas) {
		for (char c : row) {
			if (c == '|') {
				std::cout << RED << c << RESET;
			}
			else if (c == '*') {
				std::cout << GREEN << c << RESET;
			}
			else if (c == '-') {
				std::cout << BLUE << c << RESET;
			}
			else {
				std::cout << c;
			}
		}
		std::cout << "\n";
	}

}


int main() {
	const int WINDOW_WIDTH = 100;
	std::deque<float> audio_history(WINDOW_WIDTH, 0.0f);

	float global_time = 0.0f;
	float time_step = 0.05f;

	while (true) {
		float sample = std::sin(global_time);

		// INTENTIONAL GLITCH: Every ~30 frames, force a hard snap
		if ((int)(global_time * 10) % 50 == 0) {
			sample = ((int)global_time % 2 == 0) ? 0.9f : -0.9f;
		}

		audio_history.pop_front();
		audio_history.push_back(sample);

		std::vector<float> current_buffer(audio_history.begin(), audio_history.end());

		std::cout << "\033[2J\033[H";
		render_wave(current_buffer);

		global_time += time_step;
		std::this_thread::sleep_for(std::chrono::milliseconds(30));
	}

	return 0;
}