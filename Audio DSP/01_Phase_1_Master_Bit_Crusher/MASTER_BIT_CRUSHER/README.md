# About
This is a very basic bit crusher implemented by learning some basic concepts of Audio DSP.

<b>NOTE:</b> This is not a professional project. This has been created just for fun. Feel free to give me suggestions on what can be improved. I would love to hear some feedback.

# Basic Conceptual Roadmap
1. Audio is received
2. A biquad filter runs as a high pass filter to clean up the DC offset (if any)
3. A biquad filters runs as a low pass filter for anti-aliasing
4. A resample then runs to crush the sample rate
5. A biquad filtr then runs again as a low pass filter to "smooth out" the sharp edges caused by resampling
6. Output