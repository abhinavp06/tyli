# Spectral Cleanup
## Noise Shaping and Error Feedback
- Noise shaping doesn't help in reducing the error caused by our processing (because of conservation of energy) but it helps distribute it
- With noise shaping, the goal is to push the quantization noise from sensitive frequencies to the high frequencies which the human ear is less sensitive to
- An error feedback loop basically keeps track of the error generated in the previous sample and subtracts it from the next sample
- Dither is added to the input, then any error from the previous sample is added (in the context of our bit crusher's error feedback system). We then quantize the sample and give the output. Then we calculate the new error, filter it and store it for the next sample.
## IIR Filter Design Leaky Integrator
- We basically have a low pass filter which contains the memory of the previous output.
- The tail of the reverb/decay gets infinitely small but never hits zero. The CPU switches to Denormal mode to maintain precision. However, this mode is about 100x slower than normal math. So we manually snap the numbers to 0.0f.
## DC Offset Correction Strategies
- Basically a constant noise
- If my signal is shifted by an offset of +0.2, sound will start clipping at +0.8 instead of +1.0. This can cause issues and loss in resolution of the final output. This will also cause a "pop" when the speaker snaps back to 0.

# Questions
1. Why is the white noise floor audible in lower bit depths? Is it because there isn't a lot of dynamic range?
2. Is sweeping the errors to the high frequencies the only way to perform noise shaping?
3. Are noise shaping mechanisms affected by bit depth? What about mid rise vs mid tread architectures? I am thinking that a lower bit depth means that the curve of us shaping the noise will be aggressive. For mid tread/mid rise, I am assuming that the slope (angle to the x-axis) will be sharper?
4. In noise shaping, why are we using a high pass filter? Shouldn't we be using a low pass filter? Won't a high pass filter exaggerate the sounds? Plus, if we are sweeping the noise from low frequencies to high frequencies, shouldn't the goal be to "clean up" the final sound in the higher frequencies?
5. Explain the use of the leaky integrator in terms of bit crusher (and all the other concepts covered before this). Why is that being used? Also explain the process of feedback and memory.
6. Explain the relation between the high pass filter in noise shaping and the low pass filter in the IIR filter.
7. What is the "zipper noise" in the IIR leaky integrator? What happens when the user turns the knob rapidly?
8. Is the DC Offset the error that is produced while processing samples? Explain in the context of all the other things we discussed and also how it relates to the bit crusher.
9. Is there any relation between the DC offset and mid rise/mid tread architectures? Do the architectures affect it? If there is a huge DC offset and a mid rise architecture, can it cause the output device to blow up even more easily?
10. If my speaker is clipping to the highest possible frequency and suddenly going to 0, I get a pop. Is it the same if my speaker is pulled in to it's maximum possible value and then goes to 0?
11. What exactly are we doing with the DC offset in terms of our bit crusher?
12. Explain biquad filter cascades and how it ties up to our bit crusher.

### 1. Why is the white noise floor audible in lower bit depths? Is it because there isn't a lot of dynamic range?

**Yes.** It is a direct mathematical consequence of **Dynamic Range**.

- The formula is approximately 6dB of range per bit.
    
- At **16-bit**, the noise floor is at −96dB (well below the threshold of hearing in a standard room).
    
- At **4-bit**, your floor is at −24dB—loud enough to compete with the music.
    
- **The Analogy:** Imagine a pixelated image. 16-bit is a 4K display (noise "pixels" are invisible). 4-bit is a low-resolution LED billboard (the "pixels" are huge blocks).
    

### 2. Is sweeping the errors to the high frequencies the only way to perform noise shaping?

**No.** While pushing noise to the highs is the most common "fixed" strategy because the ear is less sensitive there, there are **Psychoacoustic** strategies. We can shape the noise to hide under the "Masking Threshold" of the signal itself. If a loud guitar is playing at 400Hz, we can hide more noise around 400Hz because the human ear is effectively "blinded" in that specific frequency band by the louder sound.

### 3. Are noise shaping mechanisms affected by bit depth? What about mid rise vs mid tread architectures? I am thinking that a lower bit depth means that the curve of us shaping the noise will be aggressive. For mid tread/mid rise, I am assuming that the slope (angle to the x-axis) will be sharper?

- **Bit Depth:** It doesn't change the _angle_ (slope) of the curve, but it changes the _volume_ of the noise. A 1st-order shaper always has a 6dB/octave slope. At 4-bit, you are just sweeping a much bigger pile of dirt.
    
- **Architectures:** Mid-Rise creates a constant "DC error" during silence. A noise shaper might try to "shape" this constant offset, leading to a "thumping" or "breathing" noise floor at low frequencies. The slope stays the same, but the starting point of the error is shifted.
    

### 4. In noise shaping, why are we using a high pass filter? Shouldn't we be using a low pass filter? Won't a high pass filter exaggerate the sounds? Plus, if we are sweeping the noise from low frequencies to high frequencies, shouldn't the goal be to "clean up" the final sound in the higher frequencies?

**CRITICAL CORRECTION:** You must separate the **Audio** from the **Error**.

- We are **High-Passing the Error**, not the music.
    
- **The Goal:** Human hearing is most sensitive between 2kHz and 5kHz. It is very sensitive to low-frequency "rumble." It is least sensitive to very high-frequency "hiss."
    
- By using a High-Pass on the error, we ensure the noise lives in the high frequencies (Hiss) where we mind it the least. If we used a Low-Pass on the error, we would have a constant, muddy rumble in the bass.
    

### 5. Explain the use of the leaky integrator in terms of bit crusher (and all the other concepts covered before this). Why is that being used? Also explain the process of feedback and memory.

- **Role:** It is the "Tone" or "Warmth" control.
    
- **Application:** Bit crushing produces "staircase" waves with sharp vertical edges. In physics, an instant vertical jump requires infinite energy and creates infinite high-frequency harmonics (the "digital scream").
    
- **Feedback & Memory:** The Leaky Integrator uses the previous output (y[n−1]) to calculate the current one. This creates **Inertia**. The signal can no longer jump vertically; it must "leak" toward the new value. This rounds off the sharp corners, making the bit-crushed signal sound like an old analog sampler rather than a broken computer.
    

### 6. Explain the relation between the high pass filter in noise shaping and the low pass filter in the IIR filter.

**They are in a "Push-Pull" relationship.**

- The **Noise Shaper** (High-Pass logic) pushes the quantization noise into the upper frequencies.
    
- The **Leaky Integrator** (Low-Pass) then cuts those same high frequencies.
    
- **Result:** If you place the Leaky Integrator _after_ the Bit Crusher, it actually helps remove the noise that the shaper pushed up there. It results in a very "dark" but "clean" lo-fi sound.
    

### 7. What is the "zipper noise" in the IIR leaky integrator? What happens when the user turns the knob rapidly?

- **Scenario:** You have a digital volume knob. The computer updates it in steps (e.g., 0.5→0.6).
    
- **The Artifact:** If the knob moves instantly, the audio waveform "jumps," creating a tiny click. If you turn the knob fast, you get a series of rapid clicks that sound exactly like a zipper being pulled (`zzzip`).
    
- **The Fix:** You run the _knob's value_ through a Leaky Integrator. Instead of jumping from 0.5 to 0.6, the value "slides" through 0.51,0.52,0.53... This makes the parameter change smooth and silent.
    

### 8. Is the DC Offset the error that is produced while processing samples? Explain in the context of all the other things we discussed and also how it relates to the bit crusher.

**Yes.** In a perfect world, an audio wave oscillates equally above and below the zero line. In a Bit Crusher, your quantization (especially if using truncation or Mid-Rise) often "favors" one direction. This shifts the "center of gravity" of the wave. That shift is the **DC Offset**. It is a constant voltage error added to your signal.

### 9. Is there any relation between the DC offset and mid rise/mid tread architectures? Do the architectures affect it? If there is a huge DC offset and a mid rise architecture, can it cause the output device to blow up even more easily?

- **Relationship:** Mid-Rise **guarantees** DC offset during silence because it cannot represent 0.0. It must choose +0.5 or −0.5 (in bit-steps).
    
- **Hardware Impact:** It won't "explode" the speaker like a bomb, but it creates **Heat**. The speaker coil is energized to hold the cone at that offset position. Over time, this constant current heats the coil, reduces life span, and eats up your "Headroom" (you clip sooner because the cone is already halfway pushed out).
    

### 10. If my speaker is clipping to the highest possible frequency and suddenly going to 0, I get a pop. Is it the same if my speaker is pulled in to it's maximum possible value and then goes to 0?

**Yes.** A "pop" is just a massive, instantaneous change in air pressure.

- A jump from +1.0→0.0 is a massive displacement.
    
- A jump from −1.0→0.0 is also a massive displacement.
    
- Your ear doesn't care about the direction; it only cares that the "jump" happened instantly.
    

### 11. What exactly are we doing with the DC offset in terms of our bit crusher?

We are **neutralizing it**. We place a **DC Blocker** (a very specialized High-Pass filter at roughly 20Hz) at the final stage of the Bit Crusher. It ignores the fast-moving audio but "pulls" any steady, constant offset back to the zero line.

### 12. Explain biquad filter cascades and how it ties up to our bit crusher.

- **The Concept:** A Leaky Integrator is a "1st-order" filter (gentle 6dB/octave slope). A Biquad is "2nd-order" (12dB/octave).
    
- **The Cascade:** Chaining Biquads creates "high-order" filters with incredibly steep slopes (e.g., 24dB or 48dB/octave).
    
- **The Goal:** We use cascades in our Bit Crusher to simulate high-end hardware. Legendary samplers have very sharp output filters that "choke" the high frequencies in a specific way. Chaining Biquads lets us build that "brick wall" to get that punchy, vintage sound.