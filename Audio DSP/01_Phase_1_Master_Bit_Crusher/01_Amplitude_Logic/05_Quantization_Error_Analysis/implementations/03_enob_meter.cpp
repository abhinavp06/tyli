/*
* The ENOB Meter
* ENOB -> Effective Number of Bits
* A bit crusher may be set to 8-bits but because of internal math errors, noise, or jitter, the audio might effectively sound like 6-bits.
* Goal -> Calculate the RMS of the error sgnal to determine the actual bit depth.
* $SNR_{dB} = 6.02N + 1.76$. We measure the Noise (from the Null Tester), calculate SNR, and solve for $N$ (Bits).
* Success metric:
*		A realtime meter telling exactly how much precision is being lost.
*/

#include <cmath>
#include <vector>
#include <numeric>

class ENOBMeter {
public:
    // Call this every sample with the CLEAN input and the PROCESSED output
    void feed_sample(float clean_input, float processed_output) {
        // 1. Isolate the Error (The Sawdust)
        float error = processed_output - clean_input;

        // 2. Accumulate Energy (Squared)
        sum_squared_error += (error * error);
        sample_count++;
    }

    // Call this once per block (e.g., every 512 samples) to get the reading
    float get_enob_reading() {
        if (sample_count == 0) return 0.0f;

        // 1. Calculate RMS of the Error (Noise Floor)
        float mean_square = sum_squared_error / sample_count;
        float rms_error = std::sqrt(mean_square);

        // Safety: Prevent log(0)
        if (rms_error < 1e-9f) return 32.0f; // Perfect silence = Infinity bits

        // 2. Calculate SNR (Signal to Noise Ratio)
        // We assume a Full Scale Sine Wave reference (0dBFS = 1.0 amplitude)
        // Ideally, we'd measure the Signal RMS too, but for this test, we assume Full Scale.
        // SNR = 20 * log10(Signal_RMS / Noise_RMS)
        // For a Full Scale Sine, Signal RMS is 0.707 (1/sqrt(2)).
        float signal_rms = 0.7071f;
        float snr_db = 20.0f * std::log10(signal_rms / rms_error);

        // 3. Solve for N (Bits)
        // SNR = 6.02 * N + 1.76
        // N = (SNR - 1.76) / 6.02
        float enob = (snr_db - 1.76f) / 6.02f; // standard formula $$SNR_{dB} = 6.02N + 1.76$$

        // Reset for next block
        sum_squared_error = 0.0f;
        sample_count = 0;

        return enob;
    }

private:
    float sum_squared_error = 0.0f;
    int sample_count = 0;
};