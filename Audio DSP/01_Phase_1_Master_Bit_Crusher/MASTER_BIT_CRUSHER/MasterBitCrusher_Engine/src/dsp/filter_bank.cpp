#include "filter_bank.h"
#include <cmath>

void FilterBank::init(double sampleRate) {
    sampleRate_ = sampleRate;

    // DC Blocker: High-Pass @ 20Hz, Q=0.707
    dcBlocker_.makeHighPass(sampleRate, 20.0, 0.707);

    // Anti-Alias: Low-Pass @ 2000Hz (just below new Nyquist for decimation by 10)
    // Q=0.707 (Butterworth - maximally flat)
    antiAliasFilter_.makeLowPass(sampleRate, 2000.0, 0.707);

    // Post-Filter: Low-Pass @ 5000Hz (gentle tone shaping)
    postFilter_.makeLowPass(sampleRate, 5000.0, 0.707);
}

double FilterBank::processDCBlocker(double input) {
    return dcBlocker_.process(input);
}

double FilterBank::processAntiAlias(double input) {
    return antiAliasFilter_.process(input);
}

double FilterBank::processPostFilter(double input) {
    return postFilter_.process(input);
}

void FilterBank::reset() {
    dcBlocker_.reset();
    antiAliasFilter_.reset();
    postFilter_.reset();
}