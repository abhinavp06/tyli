#include "stereo_manager.h"

void StereoManager::setMode(Mode mode)
{
    mode_ = mode;
}

void StereoManager::encode(double left, double right, double &mid, double &side)
{
    mid = (left + right) / 2.0;  // Center content
    side = (left - right) / 2.0; // Stereo width
}

void StereoManager::decode(double mid, double side, double &left, double &right)
{
    left = mid + side;
    right = mid - side;
}