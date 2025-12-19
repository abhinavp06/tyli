I will now build a #term_BitCrusher.

The first code [[glitch_simulator.cpp]] generated a glitch by telling a sample to a random location. A bit crusher will force every sample to teleport to the nearest "legal" grid line. This gives the hazy and muddy sound. ^7168da

I know for a bit crusher I need to accept a stream of bits and then I need to randomly hold a value for a particular duration. -> This is actually a #term_SampleRateReducer or a #term_Downsampler ( #definition_term_SampleRateReducer #definition_term_Downsampler)
# The Principle
A Bit Crusher destroys #term_AmplitudeResolution. It forces a smooth wave into a "staircase" by rounding values to the nearest LSB.

#definition_term_LeastSignificantBit is the smallest "step". In a 3-bit system, we have 8 "steps" (aka the #term_BitDepth . The distance between step 1 and step 2 is 1 LSB.

#definition_term_ErrorSignal is the difference (the [[tyli/002_19_12_2025/note#^7168da|mud]])  between the original and the quantized. For example if the original is 0.707 and the quantized is 0.75, the error is 0.043. This error is what we hear as 'mud'.

# Analogy #analogy_term_BitCrusher

^6de607

Imagine you need to build a smooth high resolution photograph of a sunset. In the original photo, there are millions of colors. The transitions between colors are smooth and you cant see where one color ends and the next begins. This is your 16-bit or 24-bit audio.

Now imagine you want to build this with Legos. You are forced to use a fixed baseplate. You can only place a brick exactly where the "studs" are. You only have 8 bits (if the bit crusher is set to 3 bits, the total number of choices are $2^{3}$). 

We use $2^{N-1}$ in the code because audio is bipolar.
	The positive side: 4 levels (0,1,2,3,4)
	 The negative side: 4 levels (-1,-2,-3,-4)
	 Total: 8 levels ^6c310b

So, in the Lego analogy, if you have a **3-bit** Lego set, you literally only have **8 different heights** of bricks to build your wave. If the "real" wave height is $3.4$, you don't have a brick for that. You have a "3" brick and a "4" brick. You have to choose.

The mud is created with the difference (0.4) in the case above. This doesn't just disappear. In audio, this difference is converted to a new sound because the gap is created by the brick itself, the noise is physically "glued" to the shape of the Lego. If you move the Lego sculpture, the gap moves with it. You can't have the blocky sunset without the blocky edges.
# The Physics of Failure
### The Mud
High volume signals create an error signal that is "glued" to the music. This creates #term_HarmonicDistortion
#definition_term_HarmonicDistortion It is the friction created because our Lego bricks ( #definition_term_LeastSignificantBit ) don't perfectly fit the smooth curve of the real world. Because the bricks are forced to follow the curve, the friction is correlated (glued) to the music.

### The Buzz
At low volumes, the sine wave becomes a square wave. This is a #term_LimitCycleOscillation 

### The Gate
If the signal is smaller than 0.5 LSB, it is rounded to zero. Total silence.

---

# BASIC BIT CRUSHER [[basic_bit_crusher.cpp|IMPLEMENTATION]]

Time to implement a basic bit crusher to visualize the 'mud'.

1. First create a basic grid to know how many "steps" are present in the ladder.
2. Then project it by multiplying it by $2^{N-1}$ where N is the N in N-bit audio.
3. Then round it off. This is where the mud is born.
4. Then divide it back by $2^{N-1}$


![[Pasted image 20251220005201.png]]
Basic Bit Crusher (with attached mud)
	The red bars represent the moment the error becomes so large that it jumps to a new row. The mud is physically attached to the signal.

![[Pasted image 20251220005252.png]]
Basic Bit Crusher (Only mud)
	We have removed the "music" and are only showing the mud.
	This is #term_HarmonicDistortion because the mud is created by rounding the sine wave, the mud looks like the sine wave.  ^39a1c1
	 The spikes represent the size of the gap between the Lego brick and the real photo (see the [[tyli/002_19_12_2025/note#^6de607|note]]) When the photo is at eight 3.5, the gap is 0.5. When the photo is at height 3.0, the gap is 0. This visual only shows the gaps. The spikes form a pattern because the gaps depend on the shape of the original sunset.

# QUESTIONS TO REVISIT #tag_QuestionsToRevisit

### In [[tyli/002_19_12_2025/note#^6c310b|note]] why is the positive containing 0 and only going up to 3 but the negative goes to -4?
The #term_ZeroProblem 

### If the #term_ErrorSignal is the difference between the "Ideal" and the "Lego Brick," what happens to that Error Signal if the original sound is a perfect horizontal line that sits exactly on a "stud"?

### What happens when you increase the bit depth from 3 to 8 in the 'Only Mud' visualizer?

### We call it #term_HarmonicDistortion because of a repeating pattern. What is the term for the distortion that is not periodic?
-> Lead to white noise implementation

# WHITE NOISE [[white_noise_simulator.cpp|IMPLEMENTATION]]

Perfect white noise is simply a stream of random numbers within a range (-1.0 to 1.0 for audio).

Steps:
1. Pick a random number between -1.0 and 1.0
2. Send it to the history buffer
3. Repeat for the next sample


> [!NOTE] Noise vs. Distortion
> - **Harmonic Distortion (Mud):** Predictable error. It sounds like grit.
> - **White Noise (Hiss):** Random error. It sounds like static.
> - **The Skill:** To turn one into the other, we use **Randomness**.


# BEFORE MOVING ON
### 1. The Decision Boundary (The "Split")
In your current code, `std::round(0.4)` becomes `0`, and `std::round(0.6)` becomes `1`. The **0.5 mark** is your decision boundary.
- **The Concept:** Quantization is essentially drawing a line in the sand.
- **The Problem:** Depending on _where_ you draw that line, silence (0.0) behaves differently.
- **Relevance:** This is the "DNA" of the Mid-Rise vs. Mid-Tread debate. One architecture puts a "flat step" at zero, and the other puts a "split" at zero.
### 2. Quantization Error (The "V" Shape)
You’ve seen the "Mud" spikes. Now, look at the **distribution** of that error.
- If your input is $0.5$ and your output is $1.0$, your error is $+0.5$.
- If your input is $1.4$ and your output is $1.0$, your error is $-0.4$.
- **The Principle:** The error signal always stays within the range of $\pm 0.5$ LSB.

 >[!NOTE] "The maximum possible Mud is always half of one Lego brick which is equal to  ($0.5 \times LSB$)."
### 3. The Transfer Function (The "Staircase" Map)
You need to be comfortable with the term **Transfer Function**. It’s just a fancy name for the graph of `Input vs. Output`.
- A "Perfect" system is a straight diagonal line (Input = Output).
- Your "Bit Crusher" is a **Staircase**.
- **Why it matters:** Mid-Rise and Mid-Tread are simply two different ways of drawing that staircase.