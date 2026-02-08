#include "../dsp/quantizer.h"
#include "../dsp/dither.h"
#include "../utils/oscillator.h"
#include "../utils/csv_writer.h"

int main() {
    Oscillator osc;
    osc.init(44100.0);
    osc.setFrequency(100.0);

    Quantizer quant;
    quant.setBitDepth(4.0);
    quant.setArchitecture(Quantizer::Architecture::MidTread);

    Dither dither;
    dither.setAmplitude(4.0);
    
    // Enable for mastering/archival. Disable for creative lo-fi.
    dither.setNoiseShaping(true);

    CsvWriter csv;
    csv.open("dither_test.csv");
    csv.writeHeader({ "Sample", "Input", "NoDither", "WithDither", "Error" });

    for (int i = 0; i < 500; ++i) {
        double input = osc.nextSample();

        double noDither = quant.process(input);

        double ditherNoise = dither.process(0.0, input);
        double withDither = quant.process(input + ditherNoise);

        double error = withDither - input;

        csv.writeRow({ (double)i, input, noDither, withDither, error });
    }

    csv.close();
    return 0;
}