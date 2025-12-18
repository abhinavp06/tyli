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
    if (value > 1.0f) value = 1.0f;
    if (value < -1.0f) value = -1.0f;

    int result = std::round((1.0f - value) * (TOTAL_ROWS / 2));

    if (TOTAL_ROWS % 2 == 0) result -= 1;

    if (result > BOTTOM_ROW) return BOTTOM_ROW;
    if (result < TOP_ROW) return TOP_ROW;
    return result;
}

void renderWave(const std::vector<float>& buffer) {
    std::vector<std::string> canvas(TOTAL_ROWS, std::string(buffer.size(), ' '));
    canvas[ZERO_AUDIO_ROW] = std::string(buffer.size(), '-');

    int previousRow = -1;

    for (int i = 0; i < buffer.size(); i++) {
        int targetRow = coordinate_mapper(buffer[i]);

        if (previousRow != -1) {
            int distance = std::abs(targetRow - previousRow);
            if (distance > GLITCH_THRESHOLD) {
                int start = std::min(previousRow, targetRow);
                int end = std::max(previousRow, targetRow);
                for (int r = start + 1; r < end; r++) {
                    canvas[r][i] = '|';
                }
            }
        }

        canvas[targetRow][i] = '*';
        previousRow = targetRow;
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
    std::deque<float> audioHistory(WINDOW_WIDTH, 0.0f);

    float globalTime = 0.0f;
    float timeStep = 0.05f;

    while (true) {
        float sample = std::sin(globalTime);

        // INTENTIONAL GLITCH: Every ~30 frames, force a hard snap
        if ((int)(globalTime * 10) % 50 == 0) {
            sample = ((int)globalTime % 2 == 0) ? 0.9f : -0.9f;
        }

        audioHistory.pop_front(); 
        audioHistory.push_back(sample);

        std::vector<float> currentBuffer(audioHistory.begin(), audioHistory.end());

        std::cout << "\033[2J\033[H";
        renderWave(currentBuffer);

        globalTime += timeStep;
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
    }

    return 0;
}