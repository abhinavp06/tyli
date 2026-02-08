#include "../dsp/biquad.h"
#include "../utils/oscillator.h"
#include "../utils/csv_writer.h"

int main() {
    // Generate a test signal: 100Hz + 5000Hz (two tones)
    Oscillator osc1, osc2;
    osc1.init(44100.0);
    osc2.init(44100.0);
    osc1.setFrequency(100.0);  // Bass
    osc2.setFrequency(5000.0); // Treble

    // Create a Low-Pass Filter @ 1000Hz
    Biquad lowPass;
    lowPass.makeLowPass(44100.0, 1000.0, 0.707); // Q=0.707 (Butterworth)

    CsvWriter csv;
    csv.open("biquad_test.csv");
    csv.writeHeader({ "Sample", "Input", "Output" });

    for (int i = 0; i < 1000; ++i) {
        double bass = osc1.nextSample() * 0.5;
        double treble = osc2.nextSample() * 0.5;
        double input = bass + treble; // Mix both tones

        double output = lowPass.process(input);

        csv.writeRow({ (double)i, input, output });
    }

    csv.close();
    return 0;
}