# About
This is a very basic bit crusher implemented by learning some basic concepts of Audio DSP.

<b>NOTE:</b> This is not a professional project. This has been created just for fun. Feel free to give me suggestions on what can be improved. I would love to hear some feedback.

# Basic Conceptual Roadmap
1. Audio is received
2. A biquad filter runs as a high pass filter to clean up the DC offset (if any)
3. A biquad filters runs as a low pass filter for anti-aliasing
4. A resample then runs to crush the sample rate
5. A biquad filter then runs again as a low pass filter to "smooth out" the sharp edges caused by resampling
6. Output

# Run Program
```
cd MasterBitCrusher_Engine
./build.sh
```

## Test Cases
### Test 1: Static Bit Crusher (Classic)
```
./build/bitcrusher input.flac output_static.wav
# When prompted:
# Dynamic Bit Depth? 0
# Static Bit Depth: 8
# Sample Rate Divisor: 1
# Use Boxcar? 1
```
### Test 2: Dynamic Bit Crusher ("Dying Battery Effect")
```
./build/bitcrusher input.flac output_dynamic.wav
# When prompted:
# Dynamic Bit Depth? 1
# Base Bit Depth: 4
# Max Modulation Depth: 12
# Sample Rate Divisor: 1
# Use Boxcar? 1
```
### Test 3: Extreme Decimation
```
./build/bitcrusher input.flac output_lofi.wav
# When prompted:
# Dynamic Bit Depth? 0
# Static Bit Depth: 8
# Sample Rate Divisor: 10  # ← Heavy lo-fi
# Use Boxcar? 0  # ← Naive (harsh aliasing)
```

# Some Questions
1. What about our analysis tools (in the "analysis" sub directory)? enob_meter.h, glitch_detector.h, linearity_scope.h, null_tester.h?
2. I fixed the anti aliasing bug. I would like to add the noise gate to my dither.
3. Yes, I want to add polish features.
4. When will we tackle the stereo_manager.h file?
5. What other projects can I build with the knowledge I have? Even simple tools like flac to wav converter, white noise generator, etc. Give me a list of atleast 5-10 projects that I can make with the knowledge that I have so that I can improve my intuition in understanding all the concepts.
6. Can I decrease the amount of white noise?
7. Does a higher buffer size affect the output?