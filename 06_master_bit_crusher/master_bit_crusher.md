# 🧠 Project: Noise Shaping (Error Feedback)

> [!ABSTRACT] Goal
> 
> To implement Noise Shaping, a closed-loop DSP technique that filters quantization error. By taking the "rounding error" from one sample and subtracting it from the next, we "push" the audible noise out of the mid-range and into the ultra-high frequencies ($>15\text{ kHz}$), where the human ear—and your IEM drivers—are less sensitive.

---

## 🏗️ First Principle Thinking

### 1. The Conservation of Error

**First Principle:** You cannot destroy quantization noise; you can only move it.

- In a standard Bit-Crusher, the noise is **White Noise** (spread evenly across all frequencies). This sounds like a harsh hiss in the mid-range ($2\text{ kHz} - 5\text{ kHz}$), which is where our ears are most sensitive.
    

### 2. Error Feedback (The Loop)

- **Concept:** Every time we "crush" a sample, we calculate exactly how much information we lost ($Error = \text{Desired Value} - \text{Actual Value}$).
    
- **The Trick:** Instead of throwing that error away, we **save it** and add it to the _next_ incoming sample.
    
- **The Result:** This creates a **Negative Feedback Loop**. By adding the previous error to the current sample, we effectively perform a **High-Pass Filter** on the noise signal itself.
    

### 3. Psychoacoustics: Hiding the Body

- Human hearing is shaped like a "U." We have evolved to hear a twig snap in the woods (mid-range), but we are effectively deaf to sounds near $20\text{ kHz}$.
    
- Noise shaping "hides" the crunchiness in that $18\text{ kHz} - 20\text{ kHz}$ region.
    

---

## 🧪 The Mathematical Model

To implement a **First-Order Noise Shaper**:

1. Let $e[n-1]$ be the error from the previous sample.
    
2. **Modified Input:** $x'[n] = x[n] + e[n-1]$
    
3. **Quantize:** $y[n] = \text{Quantize}(x'[n])$
    
4. **Calculate new Error:** $e[n] = x'[n] - y[n]$
    

**The Difference:** In a normal crusher, $e[n]$ is discarded. Here, it becomes the "bias" for the next calculation.

## 🎨 Visualization Guide

### What to look for in your IEM Visualizer:

1. **Standard Crush:** The "staircase" looks random and jagged. The red `|` bars (glitches) appear scattered throughout the wave.
    
2. **Noise Shaped Crush:** The wave will appear to "vibrate" or "flicker" at a very high frequency.
    
3. **The Observation:** You will see the `*` points jumping back and forth much faster than the actual sine wave frequency. This is the "Noise" being pushed into high frequencies.
    

---

## 🧠 Questions for Further Thought

### 1. Why does this only work at high sample rates?

If your sample rate is low (e.g., $8\text{ kHz}$), the "high frequency" where we hide the noise is only $4\text{ kHz}$ (Nyquist). That is right in the middle of the human "screaming" frequency! Noise shaping only sounds good when the sample rate is high enough to move the noise floor above $15\text{ kHz}$.

### 2. Higher Order Shaping?

We used `lastError`. If we used a weighted average of the last _two_ errors, we would have a **Second-Order** shaper. This pushes the noise even more aggressively into the highs, creating a "cleaner" audible signal but a much more "violent" ultra-sonic signal.

### 3. Is this "Cheating"?

In DSP, we call this **Spectral Shaping**. It is the ultimate expression of first-principles thinking: realizing that you don't have to solve a problem (Noise) if you can just move the problem to a place where nobody notices it.