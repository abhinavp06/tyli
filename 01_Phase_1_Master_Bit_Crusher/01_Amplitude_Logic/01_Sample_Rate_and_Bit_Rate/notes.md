# 01_Sample_Rate_and_Bit_Rate
> **Status:** #status/active
> **Tags:** #domain/DSP #concept/digital_audio #term/SampleRate #term/BitDepth
> **Related:** [[02_Dither_and_Chaos]] | [[06_Aliasing]]

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