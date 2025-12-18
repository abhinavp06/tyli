# Idea
The core objective of this project is to implement a **Bit-Crusher**, a time-domain DSP effect that intentionally degrades audio quality. By reducing **Bit-Depth** (Amplitude Resolution) and **Sample Rate** (Time Resolution), we visualize and hear the transition from smooth "Analog" curves to jagged "Digital" stair-steps. This serves as a practical exploration of **Quantization Noise** and **Aliasing**.

# First Principle Thinking
## Phase 1: Amplitude Degradation (Bit-Depth)
**First Principle:** Digital precision is finite. A "Bit" represents a power of two levels of vertical resolution.
1. **Continuous to Discrete:** A sine wave has infinite vertical positions. To "crush" it, we must force it into a fixed number of "buckets" (levels).
2. **The Math of Rounding:** If we want 4-bit audio ($2^4 = 16$ levels), we scale the signal to a range of $0-16$, `round()` it to the nearest whole number, and scale it back.
3. **The Result:** The "rounding error" between the original and the crushed sample is physically heard as **Quantization Noise**.

## Phase 2: Temporal Degradation (Downsampling)

**First Principle:** Motion requires a specific frequency of snapshots.

1. **The "Sample-and-Hold" Logic:** To simulate a lower sample rate, we don't process every incoming sample. Instead, we capture one sample and "hold" its value for $N$ subsequent samples.
    
2. **Visual Impact:** This turns smooth horizontal movement into "blocks" of time where the value stays perfectly flat.
    
3. **The Physics:** This creates **Aliasing**, where high frequencies "fold back" into lower, metallic-sounding tones because the "camera" isn't fast enough to capture the true motion.
    

## Phase 3: The Sample-by-Sample Pipeline

**First Principle:** Low-latency DSP must happen "Just-In-Time."

1. **The Predictor State:** Unlike block-based effects, a Bit-Crusher can operate on a single `float` at a time.
    
2. **Zero-Latency:** Because we aren't waiting for a 1024-sample buffer to fill up (Frequency Domain), the output is generated the microsecond the input arrives.
    
3. **Stateless vs. Stateful:** Bit-crushing is largely "stateless" (the current sample doesn't depend on the previous one), making it the perfect entry point for real-time C++ DSP.
    

## Phase 4: Visualizing the "Digital Staircase"

**First Principle:** A visualizer should reveal the math, not just the motion.

1. **The Staircase:** In our terminal visualizer, we expect to see the `*` points forming flat horizontal plateaus followed by violent vertical jumps.
    
2. **The Red Indicator:** Because the jumps between these "stair-steps" are instantaneous, our `GLITCH_THRESHOLD` will trigger. The visualizer will light up with red `|` bars, correctly identifying the high-frequency distortion created by the crushing process.
    

---

# Implementation Steps

## Step 1: The Bit-Crusher DSP Engine

We encapsulate the logic in a class to maintain clean "Sample-by-Sample" architecture. We use `pow(2, bits)` to determine our vertical resolution.

C++

```
class BitCrusher {
public:
    int bitDepth = 4; // High degradation
    
    float process(float input) {
        float levels = std::pow(2.0f, bitDepth);
        // Normalize, Quantize, and De-normalize
        float crushed = std::round(input * (levels / 2.0f)) / (levels / 2.0f);
        return crushed;
    }
};
```

## Step 2: The Downsampling logic

To simulate a lower sample rate, we implement a "Counter" and a "Latch." We only update the `heldValue` when the counter hits the `downsampleFactor`.

C++

```
int counter = 0;
float heldValue = 0.0f;
int downsampleFactor = 4; // Only update every 4th sample

if (counter++ % downsampleFactor == 0) {
    heldValue = rawSample;
}
sampleToRender = heldValue;
```

## Step 3: Integrating the Visualizer

We reuse the **O(N) Canvas Renderer** from our previous project.

1. **The Stream:** The `main()` loop generates a sine wave.
    
2. **The Processing:** Each sample passes through the `BitCrusher`.
    
3. **The Render:** The `render_wave` function plots the results.
    
4. **The Observation:** We observe how a 2-bit crush looks like a "Square Wave" and how a 16-bit crush looks identical to the original Sine wave.
    

# Questions

### Why does Bit-Crushing sound "fuzzy" or "noisy"?

What you are hearing is **Quantization Error**. When you round a sample from `0.857` to `0.8`, the `0.057` difference doesn't just disappear—it becomes a new, unintended signal. Because this error is random and "sharp," it contains high-frequency energy that sounds like white noise or "fuzz."

### What is the relationship between Bit-Crushing and Pre-Echo?

While they are different artifacts, they share a root cause: **Mathematical Approximation**.

- **Bit-Crushing:** Approximates the _Amplitude_ (Vertical).
    
- Pre-Echo: Approximates the Timing of Frequencies (Horizontal).
    
    Both result in energy appearing where it shouldn't be, creating "ghost" sounds that weren't in the original analog recording.
    

### Can I "Un-Crush" the audio?

No. Bit-crushing is a **Lossy** process. Once you round `0.857` to `0.8`, the original information (`0.057`) is deleted from the universe. This is why high-end IEMs and 24-bit DACs are so expensive—they aim to keep that original precision intact so you don't hear the "digital stairs."

Would you like to extend this project by adding a **Low-Pass Filter** to see if we can "smooth out" the digital stairs we just created?