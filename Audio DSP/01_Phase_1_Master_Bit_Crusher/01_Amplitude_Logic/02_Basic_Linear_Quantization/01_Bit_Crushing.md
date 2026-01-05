# Introduction
A #term_BitCrusher is actually 2 effects working in tandem"
1. #term_DownSampling
	1. We force the system to "hold" a single sample value for multiple clock cycles.
	2. If our sample rate is 48kHz and we downsample by a factor of 2, we are effectively telling the system to only care about the sound every 1/24000th of a second.
	3. This results in a staircase across the horizontal axis and introduces #term_Aliasing 
2. #definition_term_Quantization 
	1. We reduce the number of possible "levels" on the amplitude ladder.
	2. If we want a 3 bit audio, we only allow ${2^3 = 8}$ possible values for the speaker position.
	3. This results in a staircase look on the vertical axis and introduces #term_QuantizationNoise (which sounds like a fuzzy, distorted halo around the sound)
It is basically a glitch generator. By creating thousands of tiny "teleportation" events per second, we force the speaker to create high frequency square wave-like harmonics. The "crunch" is literally the sound of the speaker trying to reproduce an infinite number of tiny vertical edges.
# [[01_basic_bit_crusher_simulator.cpp |BASIC BIT CRUSHER SIMULATOR]]
I wanted to create a simple visualization for the bit crusher.

1. First create a basic grid to know how many "steps" are present in the ladder.
2. Then project it by multiplying it by $2^{N-1}$ where N is the N in N-bit audio.
3. Then round it off. This is where the mud is born.
4. Then divide it back by $2^{N-1}$

This focuses entirely on #term_BitDepth. It rounds the input signal to the nearest "rung" (level). When we round the signal, we lose the subtle nuance between levels. This lost information doesn't disappear. It actually turns into #term_QuantizationNoise .

![[tyli/01_Phase_1_Master_Bit_Crusher/01_Amplitude_Logic/02_Basic_Linear_Quantization/01_image_1.png]]
	_Basic Bit Crusher (with attached mud)_
		- The red bars represent the moment the error becomes so large that it jumps to a new row. The mud is physically attached to the signal.
		- This is the #term_QuantizedSignal
		- Ideally if the bit crusher was off, we would see a sine wave.
		- Then the error signal is attached to this ideal curve and we get this jagged look.
		 This jagged look is the visual evidence for the error signal being on top of the music.

# [[02_blocky_bit_rate_simulator.cpp|BLOCKY BIT RATE SIMULATOR]]
This is more of a Sample-and-Hold processor. Instead of rounding the volume, we hold a value for a duration of time (BLOCK_SIZE);

The blocky effect is achieved by holding the ```value_to_stretch``` variable for BLOCK_SIZE samples. We are effectively lowering the #term_SampleRate.

## Jitter
The #term_Jitter is basically a value which determines how long I need to hold a value for.
By making block sizes random, we turn the harmonic aliasing into white noise (which sounds like air).

### Dither
In the initial bit crusher, if a signal is too quite to reach the next "rung" it just gets cut off. This is called #term_TruncationDistortion.
#term_Dither fixes this by adding a tiny bit of noise before we round the number. It shakes the signal so that on average, the signal still crosses the quantization thresholds. We are trading a low-level "hiss" for a much more accurate representation of the original sound's character.


# COMPARISON

| **Feature**            | **Bit Crusher**                    | **Blocky/Resampler**                  |
| ---------------------- | ---------------------------------- | ------------------------------------- |
| **Primary Target**     | Y-Axis (Amplitude/Volume)          | X-Axis (Time/Sample Rate)             |
| **Math Operation**     | `std::round`                       | Sample-and-Hold (`stretch_countdown`) |
| **Resulting Artifact** | Quantization Noise (Fuzz)          | Aliasing (Metallic Ringing/Blocks)    |
| **Key Innovation**     | Visual feedback of the "Staircase" | Dither and Jitter for "Analog" feel   |
