#include "../dsp/resampler.h"
#include "../utils/oscillator.h"
#include "../utils/csv_writer.h"

int main() {
    Oscillator osc;
    osc.init(44100.0);
    osc.setFrequency(440);

    Resampler naiveResampler, boxcarResampler;
    naiveResampler.setRateDivisor(10);
    boxcarResampler.setRateDivisor(10);
    naiveResampler.setMode(Resampler::Mode::Naive);
    boxcarResampler.setMode(Resampler::Mode::Boxcar);

    CsvWriter csvNaive, csvBoxcar;
    csvNaive.open("naive_outputkhz.csv");
    csvBoxcar.open("boxcar_output.csv");

    csvNaive.writeHeader({ "Sample", "Output" });
    csvBoxcar.writeHeader({ "Sample", "Output" });

    for (int i = 0; i < 2000; ++i) {
        double input = osc.nextSample();
        double naive = naiveResampler.process(input);
        double boxcar = boxcarResampler.process(input);

        csvNaive.writeRow({ (double)i, naive });
        csvBoxcar.writeRow({ (double)i, boxcar });
    }

    csvNaive.close();
    csvBoxcar.close();
    return 0;
}