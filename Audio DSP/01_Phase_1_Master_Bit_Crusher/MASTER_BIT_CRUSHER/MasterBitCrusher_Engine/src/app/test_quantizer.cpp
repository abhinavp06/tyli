#include "../dsp/quantizer.h"
#include "../utils/oscillator.h"
#include "../utils/csv_writer.h"

int main() {
    Oscillator osc;
    osc.init(44100.0);
    osc.setFrequency(100.0);

    Quantizer quant;
    quant.setBitDepth(4.0); // Very low (16 levels) to see stairs clearly
    quant.setArchitecture(Quantizer::Architecture::MidTread);

    CsvWriter csv;
    csv.open("quantizer_test.csv");
    csv.writeHeader({ "Sample", "Input", "Output", "Error" });

    for (int i = 0; i < 500; ++i) {
        double input = osc.nextSample();
        double output = quant.process(input);
        double error = output - input;

        csv.writeRow({ (double)i, input, output, error });
    }

    csv.close();
    return 0;
}