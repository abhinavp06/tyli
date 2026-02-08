#pragma once
#include <vector>
#include <cmath>
#include <cstdint>

// BoxCar Filter aka Decimator
class BoxCarFilter {
public:
    BoxCarFilter() : m_accumulator(0.0f), m_counter(0), m_currentOutput(0.0f), m_reductionFactor(1) {}

    // Set the "Crush" amount. 
    // 1 = No reduction (48k -> 48k)
    // 10 = 48k -> 4.8k
    void setReductionFactor(int factor) {
        if (factor < 1) factor = 1;
        m_reductionFactor = factor;
        // Don't reset counter here to avoid clicks during automation
    }

    // The Process Block
    // Returns the decimated sample, held to match host rate
    float process(float input) {
        // 1. Accumulate the energy of the incoming stream
        m_accumulator += input;
        m_counter++;

        // 2. Check if we have gathered enough samples to form a "Block"
        if (m_counter >= m_reductionFactor) {

            // 3. Calculate the Average (The Anti-Aliasing Step)
            // We divide by the count to normalize amplitude.
            m_currentOutput = m_accumulator / static_cast<float>(m_counter);

            // 4. Reset for the next block
            m_accumulator = 0.0f;
            m_counter = 0;
        }

        // 5. Output the last calculated average (Sample and Hold behavior)
        // This creates the "Staircase" waveform
        return m_currentOutput;
    }

private:
    float m_accumulator;     // Stores the sum of samples in the block
    int m_counter;           // Counts how many samples we've collected
    float m_currentOutput;   // The value we are currently "holding"
    int m_reductionFactor;   // 'M' in the math
};