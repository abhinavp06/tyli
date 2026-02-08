#pragma once

/**
 * @brief Stereo Processing Utilities
 * Handles M/S encoding, decoding, and per-channel decorrelation
 */
class StereoManager
{
public:
    enum class Mode
    {
        Linked,      // Both channels get identical processing
        Independent, // L and R processed separately (dual-mono)
        MidSide      // M/S encoding, process separately, decode back
    };

    void setMode(Mode mode);

    // M/S Encoding (call before processing)
    void encode(double left, double right, double &mid, double &side);

    // M/S Decoding (call after processing)
    void decode(double mid, double side, double &left, double &right);

private:
    Mode mode_ = Mode::Linked;
};