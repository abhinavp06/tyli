# Projects
1. [[01_basic_glitch_simulator.cpp]]
2. [[02_block]]
---

## 1. The Executive Summary
Digital audio is an approximation of reality. We attempt to capture a continuous, fluid phenomenon (sound pressure) by forcing it onto a rigid 2D grid:
1.  **X-Axis (Time):** Sliced into snapshots called **Sample Rate**.
2.  **Y-Axis (Amplitude):** Sliced into measurement levels called **Bit Depth**.

> [!ABSTRACT] The River & The Bucket Analogy
> Imagine you are standing next to a flowing river (the **Continuous Audio Stream**). You cannot process every single water molecule individually because the river flows too fast.
> 
> Instead, audio processing works like a bucket brigade:
> 1.  **The Scoop:** You scoop up a bucket of water (a **Buffer** of samples).
> 2.  **The Work:** You pour dye into it, filter it, or shake it (this is your **DSP `processBlock`**).
> 3.  **The Release:** You pour it back into the river downstream.
> 
> **The Constraint:** You have a tiny slice of time to do your work before the river runs dry downstream. If you take too long, the flow stops, creating a "Glitch."

---

## 2. The Coordinates: Time vs. Amplitude

### A. Sample Rate (The X-Axis)
The number of snapshots taken per second to measure air pressure.
* **Standard:** 44,100 Hz or 48,000 Hz.
* **The Physics:** Time is captured in discrete ticks.
* **The Limit (Nyquist):** The sample rate determines the **Frequency Bandwidth**. You can only capture frequencies up to half the sample rate ($F_s / 2$).

> [!QUESTION] Does higher sample rate mean "more detail"?
> **Partially.** In photography, more pixels = sharper image. In audio, 44.1kHz is mathematically sufficient to capture **everything** a human can hear (20Hz - 20kHz) due to the **Nyquist-Shannon Sampling Theorem**. 
> 
> Going higher (e.g., 192kHz) doesn't make the audible range "crisper"; it simply allows you to record ultrasonics (sound bats hear). However, higher sample rates *do* push the **Aliasing** fold-over point further away, which is useful for heavy processing like saturation.

### B. Bit Depth (The Y-Axis)
The resolution of the "ruler" used to measure the height of the wave in each snapshot.
* **The Physics:** Determines the **Dynamic Range** (Signal-to-Noise Ratio).
* **The Limit:** Low bit depth results in **Quantization Noise** (a grainy, fuzzy texture).
    * 16-bit = 65,536 levels (CD Quality).
    * 24-bit = 16.7 million levels (Studio Standard).

---

## 3. The Flow: Buffers and Latency
The CPU cannot efficiently process samples one by one (44,000 interruptions a second would crash the system). Instead, we process chunks called **Blocks** or **Buffers**.

### The Buffer Size
The number of samples in one "bucket" passed to the `processBlock` function.
* **Common Sizes:** 128, 256, 512, 1024 samples.

### The Latency Math
Latency is strictly mathematical: Time is Distance divided by Speed.
$$
\text{Latency (seconds)} = \frac{\text{Buffer Size}}{\text{Sample Rate}}
$$

*Example: 512 samples at 48kHz:*
$$
\text{Latency} = \frac{512}{48000} \approx 10.6\text{ms}
$$

> [!TIP] The Tradeoff
> * **Small Buffer (128 samples):** Low latency (good for live playing), but high CPU stress. Risk of clicks if the CPU "hiccups."
> * **Large Buffer (1024 samples):** High stability (good for mixing), but noticeable delay between pressing a key and hearing sound.

---

## 4. Failure Mode 1: The Glitch (Discontinuity)
A "click" or "pop" (often caused by buffer underruns or bad editing) is physically a **Discontinuity**. It is the sound of **Physics fighting Mathematics**.

While a computer can change a value from `1.0` to `0.0` instantly, the physical world cannot.

### Layer 1: The Mechanical Reality (Teleportation)
Newton's Second Law: $F = m \cdot a$.
If the speaker cone is at `+1.0` and the next sample is `0.0`:
1.  **Distance:** Large.
2.  **Time:** Zero.
3.  **Velocity:** Infinite.
4.  **Force Required:** Infinite.
*Result:* The speaker panics and snaps back as fast as its physical suspension allows, creating a shockwave.

### Layer 2: The Spectral Reality (Infinite Frequencies)
To draw a perfectly vertical line (a step function) on a graph using sine waves, you need to sum **every frequency that exists**.
$$
\text{Step Function} \approx \sin(x) + \sin(3x) + \sin(5x) + \dots + \sin(\infty x)
$$
*Result:* A click is actually a burst of **White Noise** lasting a fraction of a millisecond.

> [!CHECK] The Solution: Zero Crossings
> To avoid clicks, we only cut or loop audio when the waveform crosses the center line (Amplitude = 0). At this point, the speaker is at rest, so no "teleportation" is required.

---

## 5. Failure Mode 2: Aliasing (The Lie)
What happens if you try to record a pitch higher than the Nyquist Limit ($F_s/2$)? The system doesn't just ignore it; it lies.

### The Wagon Wheel Effect
1.  **Reality:** A car wheel spins forward at 100 mph.
2.  **The Sample Rate:** A camera films at 24 fps.
3.  **The Artifact:** The wheel appears to spin **backward**.

In audio, if you input a 30kHz sine wave into a 48kHz system (Limit = 24kHz), the sampler takes snapshots at the "wrong" times. When it connects the dots, it draws a **new, lower frequency wave** that never existed in reality. This phantom frequency is an **Alias**.

---
## 6. Reference & Resources
* **Internal:** [[01_Process_Block]]
* **External:** [Zero Crossing Visualization (YouTube)](https://www.youtube.com/watch?v=9bwT9f9figg)

---
----
# OLD NOTES

---
tags:
  - SampleRate
  - BufferSize
  - "#Latency"
---
The Sample Rate and Buffer Size define the "resolution" and "speed" of the bucket brigade ( #analogy_audiostream See [[01. Process Block]])

# Sample Rate
- The number of snapshots (air pressure moving) of audio taken per second. #term_SampleRate ^4addf9
	- Standard is 44,100Hz or 48,000Hz.
	- Why is  this important? ^f91169
		- Imagine every one of those 44,100 snaps are laid out on a graph as individual points. If more snapshots are captured, we get more individual details. 
		- If you can get more snapshots in a second, you get more detail which can emulate sound even better.  ^e98c24
		- Imagine an infinite sample rate (don't think too much into this). If we captured the sound coming from an instrument to our recording device with an infinite sample rate, we would essentially be capturing the sound as it is!
- This determines the #term_FrequencyBandwidth #term_NyquistLimit 

# Buffer Size
- The number of samples in one "bucket" ( #ProcessBlock call) #term_BufferSize
- Common sizes are 128, 256, 512, 1024

# Latency
- The relationship between sample rate and buffer size is #term_Latency
- The relationship is strictly mathematical. Time is distance divided by speed.
$$
	Latency (seconds) = \frac{Buffer Size (samples)}{Sample Rate (samples/sec)}
$$
- Example => If you run at 48 kHz with a buffer of 512 samples:
$$
		Latency = \frac{512}{48000} \approx 10.6ms
$$
# Tradeoffs
1. Small Buffers (Low Latency):
	- Good for playing live (e.g., guitar amp sim)
	- However, the CPU has to "wake up" and ask for a bucket very frequently. If the CPU creates a bottleneck, you get crackles.
2. Large Buffer (High Stability):
	- Good for mixing/playback
	- The CPU has plenty of time to process a large chunk, but there is a noticeable delay between pressing a key and hearing the sound

# Explaining the "Glitch" or "Pop" (See [[01. Process Block#^c0a8e5]])

^9f5fe9
A "glitch" or "pop" in audio is physically a **discontinuity**. It represents the sound of **physics fighting mathematics**. 

While a computer can easily change a number from `1.0` to `0.0` instantly, the physical world (speakers and air) cannot react instantly without chaos. We can analyze this phenomenon through three layers of reality.

---

## 1. The Mechanical Reality: The "Teleportation" Problem
A speaker cone has mass. According to **Newton's Second Law**:

$$F = m \times a$$

Where:
* $F$ = Force
* $m$ = Mass of the speaker cone
* $a$ = Acceleration

### The Scenario
* **Smooth Audio (Sine Wave):** The voltage changes gradually. The speaker cone accelerates and decelerates smoothly. The force required is finite.
* **The Glitch (Discontinuity):** Imagine the signal is at **+1.0** (fully pushed out) and the next sample is **0.0** (center).

The command to the speaker is: *"Move from position +1.0 to 0.0 in zero time."*

1.  **Velocity** = $\frac{\text{Distance}}{\text{Time}}$. If Time $\approx 0$, **Velocity is Infinite**.
2.  **Acceleration** = $\frac{\text{Velocity}}{\text{Time}}$. If Time $\approx 0$, **Acceleration is Infinite**.
3.  **Force** = $m \times \text{Infinite Acceleration} =$ **Infinite Force**.

**The Result:** Since the speaker cannot generate infinite force, it "panics." It snaps back as fast as its physical suspension allows. This violent, uncontrolled mechanical snap strikes the air molecules like a hammer, creating a shockwave. That shockwave is the "click" you hear. ^8b6411

---

## 2. The Mathematical Reality: The Step Function
In Digital Signal Processing terms, this discontinuity is known as a #term_StepFunction (or #term_HeavisideStepFunction). When you cut a sine wave at a non-zero point, you are effectively multiplying your audio by a square window that ends abruptly.

### The Slope Calculation
* **Continuous Signal:** $y[n] = \sin(\omega n)$
* **The Glitch:** At sample $k$, the value jumps from $0.8$ to $0.0$.

$$\Delta y = 0.8$$
$$\Delta t = \frac{1}{44100} \text{ seconds}$$

The slope (rate of change) is:
$$\text{Slope} \approx 35,000 \text{ units/second}$$

This massive slope is indistinguishable from an ultra-high-frequency signal.

---

## 3. The Spectral Reality: Infinite Frequencies
This is the most "scientific" reason for the sound. We can analyze the "pop" using the #term_FourierTransform, which states that any signal can be broken down into individual sine waves.

**Question:** How many sine waves do you need to add together to create a perfectly vertical line (the discontinuity)?
**Answer:** **All of them.**

To build a vertical edge, you need an **infinite sum** of sine waves at *every* frequency (low, mid, and ultra-high), all aligned in phase:

$$\text{Step Function} \approx \sin(x) + \sin(3x) + \sin(5x) + \dots + \sin(\infty x)$$

**The Result:** When you hear a "pop," you are hearing a burst of **White Noise** (energy across the entire frequency spectrum) that lasts for a fraction of a millisecond. Because it contains "infinite" high frequencies, it sounds "sharp" or "bright," like a snap or a spark.

---

## Practical Application: Zero Crossings
To avoid this physics failure, audio engineers use a technique called #term_ZeroCrossing.
* **Rule:** Only cut or loop audio when the waveform crosses the center line (Amplitude = 0).
* **Reason:** At 0, the speaker is at rest. There is no "teleportation" required to silence the audio.

> [!INFO] External Resource
> **Visualization:** [How to Fix Your Audio With "Zero Crossings"](https://www.youtube.com/watch?v=9bwT9f9figg)
> *Demonstrates the "cliff edge" phenomenon in a waveform editor.*

# Questions

## You said "If you can get more snapshots in a second, you get more detail which can emulate sound even better." ( [[02. Sample Rate, Buffer Size and Latency#^e98c24]] ). Is this true?
This is partially true. In photography, more pixels usually means a sharper image. In audio, it works differently because of the **Nyquist-Shannon Sampling Theorem**. #term_Nyquist-Shannon_Sampling_Theorem
- **The Principle:** To perfectly reconstruct a frequency, you only need a sample rate that is **twice** the highest frequency you want to hear.
- **The Reasoning:** Humans hear up to **20,000 Hz**. Therefore, a sample rate of **40,000 Hz** (or 44.1k/48k for a safety margin) is mathematically enough to capture **every single bit of detail** that the human ear is capable of perceiving.
Adding more snapshots (e.g., 192,000 Hz) doesn't make the sound "more detailed" for your ears; it just allows the system to record frequencies that only bats or scientific equipment can hear (ultra-sonics).

## You said "If we captured the sound... with an infinite sample rate, we would essentially be capturing the sound as it is!"  ( [[02. Sample Rate, Buffer Size and Latency#^f91169]] ). Is this true?
This is mathematically perfect. An **Infinite Sample Rate** is, by definition, a **Continuous Analog Signal**. In the analog world (a vinyl record or a voltage in a wire), the "sample rate" is effectively infinite because there are no gaps between moments.

In audio engineering, we distinguish between two types of "detail":
1. **Frequency Detail (Sample Rate):** This determines the "highest pitch" we can record. If you sample slowly, fast-vibrating sounds (high notes) get lost or "aliased" (distorted).
2. **Amplitude Detail (Bit Depth):** This is how precise each snapshot is. This determines the "Dynamic Range" (the difference between the quietest whisper and the loudest explosion).

-  #term_SampleRate ($F_s$): 
	- Determines the **maximum frequency** we can capture. (High $F_s$ = Higher pitches). ^ee905f
	- Also known as #term_FrequencyDetail
- #term_BitDepth: 
	- Determines the **resolution of each snapshot**. (High Bit Depth = Lower noise/More precision in volume).
	- Also known as #term_AmplitudeDetail ^b20a88

**Your intuition about "capturing the sound as it is" is the goal of every high-end audio engineer.** We use 48kHz or 96kHz not necessarily to get "more detail" in the middle of the sound, but to ensure the "edges" of the sound (the high-frequency transients) are captured accurately and without digital artifacts.

## You said "sample rate determines the maximum frequency we can capture" ( [[02. Sample Rate, Buffer Size and Latency#^ee905f]] ). Does this mean if a singer sings or an instrument is plated at a very high pitch where the frequency of the pitch is higher than the sample rate of the recording device, that particular information of the sound won't be captured?
Correct! If a sound’s frequency exceeds half of your sample rate (1$F_s/2$), the recording system essentially "goes blind" to the true nature of that sound.

But it gets even weirder: the information doesn't just "disappear"—it turns into **lies**.
In DSP, we call this phenomenon **Aliasing** ( see [[06. Aliasing]] ). Here is the first-principle breakdown of why this happens.
1. The Folding Point (Nyquist Limit)
	- To capture a wave, you must capture at least its **peak** and its **trough**. That requires **two points** per cycle.
	- If you have a sample rate of 48,000 Hz, the highest frequency you can accurately describe is **24,000 Hz**.
	- This "speed limit" is called the **Nyquist Frequency**.
2. What happens when the pitch is too high?
	Imagine a singer hits a "super-sonic" note at **30,000 Hz**, but your equipment is set to **48,000 Hz** (Nyquist = 24,000 Hz).
	Because the wave is vibrating faster than your "camera" (the sampler), the snapshots are taken at the "wrong" times. The sampler might catch the peak of the first wave, but by the time it takes the second snapshot, the wave has already finished one full cycle and is halfway through the next one.
	When the computer connects those dots, it creates a **fake, lower-frequency wave** that wasn't actually there.
3. The Analogy: The Wagon-Wheel Effect
	Have you ever watched a car commercial or a movie where the wheels of the car look like they are spinning **backward**, even though the car is moving forward fast?
	- **The Reality:** The wheel is spinning very fast clockwise.
    - **The "Sample Rate":** The movie camera is taking 24 frames per second.
    - **The Alias:** Because the wheel is spinning faster than the camera can keep up with, the "snapshots" catch the spokes in positions that make it look like a slow, backward-moving wheel.
	In audio, that "backward wheel" is a **distorted, metallic-sounding pitch** that shouldn't be there.

