# Amplitude Logic
## Sample Rate and Bit Depth
- Sample rate is the number of snapshots taken per second. Higher sample rate usually means more detail (44.1kHz is usually more than enough. anything beyond that just helps in processing techniques)
- Bit depth is the resolution/levels (the Y-axis)
### Audio Lifecycle
1. The "recorder" first records an analogue wave
2. This wave is converted to digital audio buffers by an #term_ADC (Analog-to-Digital converter)
3. The hardware has a fixed buffer size which defines the total amount of "samples" it can process at once
4. Once this buffer is full, a signal is sent to the CPU to start the "processing".
5. The CPU then calls a callback function of the programs we write and returns the altered information back to the buffer.
6. This buffer is then played out to the speakers by a #term_DAC (Digital-to-Analog converter)
## Bit Crushing
- It works with 2 effects together - downsampling and quantization
- Downsampling to n means to tell the system to only care about the sound every 1/(sample_rate/n)th of a second.
- Quantization just means to snap the audio sample to a particular level in the grid.
- Jitter means to bring variation in the duration of holding a particular sample value. By making jitter value random, we turn the sound into white noise
- Dither means to add some noise to the sample before rounding off the number. When we use jittering and hold a particular sample for a while, if we don't add noise, the output will be the same value for a while which will cause spikes (for example, if a sample that's being held is 1.0, then it will cause a step to be created of 1.0. If we add dithering, the curve becomes smoother - which obviously works better if we have a higher bit depth)
## Mid Rise vs Mid Tread
- Mid rise means to not really have a "0 line". The value of 0 is usually more/less than 0. So, this also means that the speaker is usually pushed outward/inward.
- Mid tread means that the values tend to go to 0.
## TPDF
- TPDF is a triangular probability density function
- It uses 2 random RPDF sources to get the finally value
## Quantization Error Analysis
- The null tester helps in showing only the error and allows us to see how close our processed output is to the input.
- Linearity scope allows us to check the difference between the input and output in terms of lines in a graph. A perfect system will mean that the X (input) and Y (output) will be overlapping diagonal lines.
- ENOB meter helps us see the final bit depth

# Questions
1. How does making jitter value random turn the sound into white noise?
		Imagine a Drummer.
			- **Consistent Rhythm:** He hits the snare every 1.0 seconds exactly. You hear a beat (Frequency).
		    - **Jittered Rhythm:** He hits the snare at 0.9s, then 1.2s, then 0.8s. You still hear a beat, just a sloppy one.
			- **Random Rhythm:** He hits the snare at completely random intervals between 0.0s and 0.1s. The individual hits blur together. You no longer hear a "beat"; you hear a "roar" or a "hiss."
		    - **Physics:** Frequency is defined by periodicity. Randomness is the death of periodicity. If $f = 1/T$ and $T$ is random, $f$ is undefined (broadband noise).
		    
2. In a mid rise architecture, is the speaker always pushed out? Can it be pulled in?
	**No.** It is simply **never at rest**.
	In Mid-Rise, the "Zero" bucket doesn't exist. The values closest to zero are $+0.5 \times \Delta$ and $-0.5 \times \Delta$.
	- If the noise floor is slightly positive, the speaker locks at +0.5 (Out).
	- If the noise floor is slightly negative, it locks at -0.5 (In).
	- **The Artifact:** When playback stops, you might hear a faint "click" as the speaker jumps from +0.5 back to its physical resting position (turned off).\

3. When is it useful to use mid rise vs mid tread?
	- **Mid-Tread:** 99% of Pro Audio. We want silence to be true silence (0.0).
	- **Mid-Rise:**
	    1. **Telephony ($\mu$-Law):** It maximizes information. In an 8-bit phone call, wasting a whole state just for "perfect silence" is expensive. Mid-Rise uses every bit for signal.
	    2. **Creative Bit Crushing:** You _want_ that gritty, unstable sound. The fact that it chatters between +1 and -1 creates a "fizz" that cuts through a mix.

4. Why is an offset added to mid rise or mid tread after adding the initial offset (0 for mid tread and around 0.5 for mid rise)? Also, is the initial offset always supposed to be 0.5? Can it be something else?
	  - It comes from the math of rounding.
		- `floor(x)` rounds down. 0.9 becomes 0.
		- `round(x)` rounds to nearest. 0.9 becomes 1.
		- **The Trick:** `round(x)` is mathematically identical to `floor(x + 0.5)`.
		    - **Mid-Tread (Round):** We want 0.0 to stay 0.0. We use `floor(x + 0.5)`.
		    - **Mid-Rise (Floor):** We want 0.0 to become 0.5. We use `floor(x) + 0.5` (after scaling back down).
	- _Can it be something else?_ If you use `0.3`, you create a **Biased Quantizer**. Your positive peaks will clip differently than your negative peaks. This is essentially "DC Offset Distortion."

5. Explain how the noise floor breathes with the music while using TPDF (when the random sum is divided by 2.0 instead of the 2 LSB peak-to-peak values). Why is this a fail in professional plugin testing?
		- **This is a critical concept.**
		- **Ideal TPDF:** The noise level is constant. It does not change whether the music is loud, soft, or silent. It is a steady "shhh". The brain ignores steady noise easily.
	    - **The Mistake:** If you generate random numbers but scale them wrong (e.g., dividing by 2.0 when the math demands a wider spread to cover the gap), the dither is **too quiet** to fully scramble the quantization error.
	    - **The Result:** The quantization error (distortion) is _partially_ audible. Since quantization error depends on the input signal, the "noise" gets louder when the music plays and quieter when it stops.
	    - **Visual:** Ideally, dither is a fog that covers the floor. If the fog is too thin, you see the jagged floor tiles (distortion) poking through only when the lights (signal) are on. This pumping noise is extremely annoying to the ear.

6. Why does dithering before we sample-and-hold cause the noise to get smeared horizontally?
		Think of Dither as "High Frequency Hiss" (Blue/White noise). It changes every single sample (44,100 times a second).
		- **Step 1:** You add Hiss.
		- **Step 2:** You Downsample (Hold) to 1,000Hz.
	    - **Result:** You are grabbing a random noise value and **freezing it** for 44 samples.
	    - Instead of `tst-tst-tst` (fast hiss), you get `glug-glug-glug` (slow stepped random voltages).
	    - You have shifted the noise energy from high frequencies (ignorable) to low frequencies (audible rumble).

7. Does any sort of processing cause a phase shift/shift to the right in audio?
		- **Gain/Distortion:** No.
		- **Delay:** Yes (Linear phase shift).
		- **IIR Filters (Leaky Integrator, DC Blocker):** **YES.** This is the "smeared to the right" effect. The tighter the filter, the more the phase shifts.
		- **Sample Rate Reduction:** Effectively adds delay (latency) equivalent to half the hold time on average.

8. Can there be curved lines in a Linearity Scope?
		Yes.
		- **Straight Line:** Linear Gain ($y = mx$).
		- **Curved Line:** Compression, Saturation, or $\mu$-Law Quantization ($y = x^{0.5}$).
		- **Staircase:** Bit Crushing.

9. Why does ENOB exist? Why does processing a sound cause the bit depth to differed from the desired output bit depth?
		Because "Setting" $\neq$ "Reality."
		- **Setting:** You set the variable to `float` (32-bit container).
		- **Reality:** You recorded it with a cheap $5 microphone in a noisy room. The bottom 16 bits are just room noise.
		- **ENOB:** The **Effective Number of Bits**. It tells you: "Even though this is a 32-bit file, the signal quality is so trash/distorted that it's mathematically equivalent to a 12-bit file."
		- In DSP, if your math is sloppy (accumulating rounding errors), your 64-bit `double` might only have an ENOB of 50 bits.