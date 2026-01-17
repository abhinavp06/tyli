# Phase 1: Quantization Error Analysis & Jitter Implementation

**Date:** 2026-01-17
**Tags:** #DSP #AudioProgramming #CPP #Quantization #Jitter #BitCrusher
**Status:** Phase 1 Complete

---

## 1. The Core Philosophy: "The Sawdust"
In DSP, we often focus on the signal we *want* ($y[n]$). In high-end audio development, we focus on the signal we *reject* ($e[n]$).

**The Analogy:**
* **Input:** Smooth wood log.
* **Quantizer:** The Chisel.
* **Error ($e[n]$):** The **Sawdust** on the floor.

**The Golden Rule:**
1.  **Correlated Error (Bad):** If the sawdust contains "chunks" shaped like the original wood, we have **Distortion** (Harmonics, Aliasing).
2.  **Decorrelated Error (Good):** If the sawdust is fine, random dust, we have **Noise** (Dither).

---

## 2. The Implementation: Bit Crusher 2.0
We moved from a naive implementation to a "Real-Time Safe" architecture with correct Jitter logic.

### Key Corrections
1.  **RNG State:** Fixed the `fast_random()` bug where `state` wasn't updating, causing static noise.
2.  **Jitter Logic (Sample & Hold):** Moved from calculating jitter *every sample* (inefficient/wrong) to calculating it *once per hold event* (The "Goalpost" method).
3.  **Variable Clarity:** Clarified that `block_size` in this context is actually a **Rate Divider**, not the audio buffer size.

### Final Code Snippet
```cpp
#include <cmath>
#include <cstdint>
#include <algorithm>

enum class ListenMode { Normal, Residual };

class BitCrusher {
public:
    void update_parameters(float rate_divider, float bit_depth, float jitter_amount) {
        // Vertical (Amplitude)
        levels = std::pow(2.0f, bit_depth) - 1.0f;
        if (levels < 1.0f) levels = 1.0f;
        step_size = 1.0f / levels;

        // Horizontal (Time)
        jitter = jitter_amount;
        previous_block_size = std::max(1.0f, rate_divider);

        // Init Safety
        if (target_threshold < 1.0f) target_threshold = previous_block_size;
    }

    float process_sample(float input) {
        phase_count += 1.0f;

        // --- HORIZONTAL LOGIC (Time) ---
        // The "Goalpost" Logic: We only change the rate when we hit the threshold
        if (phase_count >= target_threshold) {
            hold_value = input;
            phase_count = 0.0f;

            // Calculate NEXT Goalpost
            // Formula: Base_Rate + (Random_Offset * Intensity)
            float jitter_rand = (static_cast<float>(fast_random()) * INVERTED_U_MAX) - 0.5f; 
            target_threshold = previous_block_size + (jitter_rand * jitter);

            if (target_threshold < 1.0f) target_threshold = 1.0f;
        }

        // --- VERTICAL LOGIC (Amplitude) ---
        float processed = quantize_sample(hold_value) * step_size;

        // --- NULL TESTER ---
        if (current_mode == ListenMode::Residual) {
            // Output - Input = The Error Signal
            // Gain 256x (+48dB) to make it audible
            return (processed - input) * 256.0f;
        }

        return processed;
    }

private:
    // ... (State variables: state_1, phase_count, etc.) ...
    float fast_random() {
        state_1 = (state_1 * 1664525) + 1013904223; 
        return static_cast<float>(state_1);
    }
};
```

## 3. The Tool Belt (First Principles)

We built three diagnostic tools to analyze the "Sawdust."

### A. The Null Tester (The Ear)

- **Goal:** Isolate $e[n]$.
    
- **First Principle:** **Subtraction.**
    
    - If $Signal = Clean + Error$, then $Error = Signal - Clean$.
        
- **Implementation:** `(Output - Input) * Gain`.
    
- **Why Gain?** Quantization error is often at -48dB to -96dB. Without a "Microscope" (Gain), the human ear cannot judge the _quality_ of the noise.
    
- **What to look for:**
    
    - _Tone/Buzzing_ = Distortion (Bad).
        
    - _Steady Hiss_ = Linearized (Good).
        

### B. The Linearity Scope (The Eye)

- **Goal:** Visualize the **Transfer Function**.
    
- **First Principle:** **Mapping.**
    
    - X-Axis: Input Value.
        
    - Y-Axis: Output Value.
        
- **The Logic:**
    
    - In a perfect system, Output = Input ($y=x$). This is a straight diagonal line.
        
    - In a Quantizer, we force values to "steps." This creates a **Staircase**.
        
- **Visual Proof of Dither:** When Dither is ON, the staircase steps become blurry/fuzzy. The _average_ of that blur creates a straight diagonal line again. Dither restores linearity.
    

### C. The ENOB Meter (The Ruler)

- **Goal:** Measure **Effective Number of Bits**.
    
- **First Principle:** **Signal-to-Noise Ratio (SNR).**
    
    - Resolution is just a measure of noise floor. A 24-bit file with high noise is mathematically identical to a 12-bit file.
        
- **The Math:**
    
    $$SNR_{dB} = 6.02N + 1.76$$
    
    $$N = \frac{SNR_{dB} - 1.76}{6.02}$$
    
- **Key Insight:** Jitter (Horizontal Instability) creates noise. This noise lowers the SNR. Therefore, **Jitter lowers the Effective Bit Depth.**
    

---

## 4. Q&A: Clearing the Mental Blockages

### Q1: Why add `jitter_offset` to `target_threshold`?

**The "Metronome" Analogy.**

- `previous_block_size` is the Metronome setting (e.g., 10 samples).
    
- `jitter_offset` is the error (e.g., +2 samples).
    
- `target_threshold` is the **actual time** the next click happens ($10 + 2 = 12$).
    
- We calculate this _once_ per click. If we didn't store it in `target_threshold`, we would have to recalculate the random number every sample, which causes the "Finish Line" to teleport randomly while we are running towards it.
    

### Q2: Why add Dither? The "Flicker" Concept.

**The Problem:**

If Input is `0.5` and grid lines are `0.0` and `1.0`.

- Without Dither: The math is deterministic. `round(0.5)` is always `1.0`. Error is large and constant.
    
- With Dither: We add noise. Input becomes `0.4, 0.6, 0.55...`
    
    - `round(0.4)` -> `0.0`
        
    - `round(0.6)` -> `1.0`
        
- **The Result:** The output toggles (flickers) between 0 and 1. The **Average** over time is `0.5`. We traded "Error" for "Noise."
    

### Q3: Vertical vs. Horizontal Distortion

- **Vertical (Bits):** Changes the **Shape** of the wave (Steps). Preserves Periodicity. Result = **Harmonic Distortion** (Musical).
    
- **Horizontal (Jitter):** Changes the **Timing** of the wave (Stretching). Destroys Periodicity. Result = **Inharmonic Distortion** (Noise/Static).
    

---
