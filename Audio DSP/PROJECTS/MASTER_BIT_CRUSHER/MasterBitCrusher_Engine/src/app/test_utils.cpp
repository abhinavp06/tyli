#include "../utils/oscillator.h"
#include "../utils/csv_writer.h"

int main() {
    Oscillator oscillator;
    CsvWriter writer;

    oscillator.init(44100.0);
    oscillator.setFrequency(100.0);

    writer.open("sanity_check.csv");

    std::vector<std::string> headers = { "Index", "Sample Value" };
    writer.writeHeader(headers);

    for (int i = 0; i < 1000; ++i) {
        double sample = oscillator.nextSample();
        writer.writeRow({ (double)i, sample });
    }

    writer.close();
    return 0;
}