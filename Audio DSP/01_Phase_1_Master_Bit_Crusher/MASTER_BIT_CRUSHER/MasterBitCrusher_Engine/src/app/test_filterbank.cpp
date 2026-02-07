#include "../dsp/filter_bank.h"
#include "../utils/oscillator.h"
#include "../utils/csv_writer.h"

int main() {
    FilterBank filters;
    filters.init(44100.0);

    // Test Signal: DC offset + 100Hz + 10kHz
    Oscillator bass, treble;
    bass.init(44100.0);
    treble.init(44100.0);
    bass.setFrequency(100.0);
    treble.setFrequency(10000.0);

    CsvWriter csv;
    csv.open("filterbank_test.csv");
    csv.writeHeader({ "Sample", "Raw", "AfterDC", "AfterAntiAlias" });

    double dcOffset = 0.2; // Simulated DC offset

    for (int i = 0; i < 1000; ++i) {
        double signal = dcOffset + bass.nextSample() * 0.3 + treble.nextSample() * 0.3;

        double afterDC = filters.processDCBlocker(signal);
        double afterAA = filters.processAntiAlias(afterDC);

        csv.writeRow({ (double)i, signal, afterDC, afterAA });
    }

    csv.close();
    return 0;
}