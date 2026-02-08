#pragma once
#include <cmath>
#include <numbers>

class Oscillator {
public:
    enum class Waveform { Sine, Triangle, Sawtooth, WhiteNoise };

    void init(double sampleRate);
    void setFrequency(double hz);
    void setWaveform(Waveform wave);

    double nextSample(); // Returns -1.0 to 1.0

private:
    double phase_ = 0.0;
    double phaseInc_ = 0.0;
    double sampleRate_ = 44100.0;
    Waveform wave_ = Waveform::Sine;
};