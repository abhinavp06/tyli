#pragma once

struct BitCrusherConfig {
    // Quantizer
    double bitDepth = 16.0;
    bool midTread = true;

    // Resampler
    int rateDivisor = 1;
    double jitterAmount = 0.0;

    // Dynamics
    double attackMs = 10.0;
    double releaseMs = 100.0;
    double modulationDepth = 0.0;

    // Dither
    bool ditherEnabled = true;
    bool noiseShaping = false;
};