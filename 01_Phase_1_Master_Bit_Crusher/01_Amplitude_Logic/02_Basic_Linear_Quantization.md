# 02_Basic_Linear_Quantization
> **Status:** #status/active
> **Tags:** #project/MasterBitCrusher #phase/01 #domain/DSP #topic/Quantization
> **Related:** [[01_Sample_Rate_and_Bit_Rate]] | [[03_Dither_and_Chaos]]

---

## 1. The Executive Summary
* **The "One-Liner":** Quantization is the process of forcing a smooth, fluid signal (like water) into a specific set of stair-steps.
* **The Engineering Goal:** To implement a function that reduces the bit-depth of an audio signal, effectively lowering its "resolution."
* **The "Why":** This is the core engine of the Bit Crusher. Without this, we cannot simulate the "crunchy" sound of vintage samplers or early digital consoles.

> [!ABSTRACT] The Ruler Analogy
> Imagine measuring a person's height.
> * **Real Height:** 174.3291 cm (Analog/Continuous).
> * **The Tool:** A ruler that only has marks every 10cm.
> * **The Measurement:** You are forced to write down **170 cm**.
> 
> The difference (4.3291 cm) is lost forever. In audio, this lost information becomes **Quantization Noise**.

---

## 2. The Physics (First Principles)
**The Reality vs. The Digital:**
* **Continuous Reality:** Voltage changes infinitely smoothly. Between 0.0V and 1.0V, there are infinite states.
* **The Constraint:** A computer has finite switches ($N$ bits). It can only represent $2^N$ unique voltage levels.

**The Breakdown:**
* **Infinite Input:** $x \in [-1.0, 1.0]$ (Float).
* **Finite Output:** $y \in \{ \text{discrete steps} \}$.
* The "Space between the steps" is the **Quantization Error**.

> [!NOTE] The Intuition
> The "Grittiness" of a bit crusher comes from the **sharp edges** of the staircase. When the signal jumps instantly from one step to the next, it creates high-frequency harmonics that were not in the original sound.

---

## 3. The Math (The Engine)

**Variables:**
* $x$: Input Sample (Float)
* $bits$: Target Bit Depth (Integer)
* $L$: The Scale Factor (Number of steps)

### The Derivation
To "crush" a number, we must project it into integer space, round it, and project it back.

1.  **Step 1: Define the Resolution ($L$)**
    For a signed signal (positive and negative), the number of steps on one side of zero is:
    $$L = 2^{(bits - 1)}$$

2.  **Step 2: The Transformation**
    $$y = \frac{\text{round}(x \cdot L)}{L}$$

### The Architecture Decision: Mid-Tread vs. Mid-Rise
There are two ways to build the staircase. This is a critical design choice.

**1. Mid-Tread (The "Safe" Zero)**
* **Logic:** The staircase has a flat step ("tread") exactly at $y=0$.
* **Math:** Uses `round()`.
* **Pro:** Silence stays silent.
* **Con:** Very quiet sounds (below the first step) are gated to complete silence.

**2. Mid-Rise (The "Aggressive" Zero)**
* **Logic:** The staircase "rises" through zero. There is no step at 0.0, only $+0.5$ and $-0.5$.
* **Math:** Uses `floor(x * L) + 0.5`.
* **Pro:** No gating; quiet sounds are always preserved (as noise).
* **Con:** Silence becomes a square wave buzzing between +/- 1 LSB.



---

## 4. The Implementation (C++ / DSP)

```cpp
// 02_Basic_Linear_Quantization.cpp

#include <cmath>

/**
 * Basic Linear Quantizer
 * Implements "Mid-Tread" architecture (Standard rounding).
 */
float quantize(float sample, int bits) {
    // 1. Calculate the 'Resolution' of our grid.
    //    For 4 bits: 2^(4-1) = 2^3 = 8 levels per side.
    float steps = std::pow(2.0f, bits - 1);

    // 2. Scale Up (Project to Integer Space)
    //    Example: 0.53 * 8 = 4.24
    float scaled = sample * steps;

    // 3. The Crush (Round to nearest integer)
    //    Example: round(4.24) = 4.0
    float crushed = std::round(scaled);

    // 4. Scale Down (Normalize back to 0-1 range)
    //    Example: 4.0 / 8 = 0.5
    return crushed / steps;
}
````

> [!WARNING] Common Pitfalls
> 
> - **Bit Depth 1:** If `bits = 1`, `steps` becomes $2^0 = 1$. This turns the output into a Square Wave (Hard Clipper).
>     
> - **Integer Overflow:** While `float` handles large numbers, if you cast to `int` for optimization, ensure you don't exceed the variable's limit.
>     

---

## 5. The Experiment (Verification)

**The Setup:**

1. **Input:** 100Hz Sine Wave.
    
2. **Param:** Bit Depth = 3.
    

The Visualizer Expectation:

You should see the smooth sine wave transform into a blocky staircase.

- At 3 bits, you should see exactly 4 positive steps and 4 negative steps.
    

**The Observations:**

- **Visual:** The wave looks like pixel art.
    
- **Aural:** The sound acquires a "fuzz" or "sizzle" on top of the tone. This is the error signal.
    

> [!EXAMPLE] Audio/Visual Proof
> 
> [Insert Screenshot of Terminal Visualizer showing Staircase Waveform]

---

## 6. The Synthesis & Cliffhanger

The Flaw:

This implementation works perfectly for loud signals. But what happens when the signal gets very quiet?

If the signal amplitude is lower than the height of the first step (the Least Significant Bit), the `round()` function will continuously snap it to `0`.

- **The Effect:** The audio cuts out abruptly (Gate Effect).
    
- **The Artifact:** Just before it dies, the audio toggles between 0 and 1, creating a robotic **Limit Cycle Oscillation** or "Digital Buzz."
    

The Bridge:

To fix this "Digital Ghost," we must paradoxically add more noise to the system to preserve the signal.

- _"We must inject Chaos to save the Detail."_ -> Leads to [[03_Dither_and_Chaos]].
    

---

## 7. Reference & Resources

- **Internal:** [[01_Sample_Rate_and_Bit_Rate]]
    
- **External:**
    
    - [DSP Guide: Quantization](https://www.dspguide.com/ch3/3.htm)