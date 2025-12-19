# 🧠 Project: Noise Shaping (Error Feedback)

> [!ABSTRACT] Goal
> 
> To implement Noise Shaping, a closed-loop DSP technique that filters quantization error. By taking the "rounding error" from one sample and subtracting it from the next, we "push" the audible noise out of the mid-range and into the ultra-high frequencies ($>15\text{ kHz}$).

---

## 🏗️ First Principle Thinking

### 1. The Conservation of Error

**First Principle:** You cannot destroy quantization noise; you can only move it.

- In a standard Bit-Crusher, the noise is "White Noise" (spread evenly across all frequencies). This sounds like a harsh hiss in the mid-range (where our ears are most sensitive).
    

### 2. Error Feedback (The Loop)

- **Concept:** Every time we "crush" a sample, we calculate exactly how much information we lost ($Error = \text{Original} - \text{Crushed}$).
    
- **The Trick:** Instead of throwing that error away, we **save it** and subtract it from the _next_ incoming sample.
    
- **The Result:** This creates a **Negative Feedback Loop** that acts as a High-Pass filter for the noise.
    

### 3. The Psychoacoustic Benefit

- Human hearing is a "U" shape; we are very sensitive to $2\text{ kHz} - 5\text{ kHz}$ but almost deaf to $18\text{ kHz}$.
    
- Noise shaping "hides" the crunchiness in the $18\text{ kHz}+$ region.
    

---

## 🧪 The Mathematical Model

To implement a **First-Order Noise Shaper**:

1. Let $e[n-1]$ be the error from the previous sample.
    
2. Modified Input: $x'[n] = x[n] + e[n-1]$
    
3. Quantize: $y[n] = \text{Quantize}(x'[n])$
    
4. Calculate new Error: $e[n] = x'[n] - y[n]$
    

**The Difference:** In a normal crusher, $e[n]$ is discarded. Here, it becomes the "bias" for the next calculation.

## 🎨 Visualization Guide

### What to look for in your IEM Visualizer:

1. **Standard Crush:** The "staircase" looks random and jagged. The red `|` bars (glitches) appear scattered.
    
2. **Noise Shaped Crush:** The visualizer looks "busier." Because the error is being fed back, the wave will appear to "vibrate" at a very high frequency.
    
3. **The Observation:** You will see the `*` points jumping back and forth much faster. In a real speaker, this vibration is so fast ($22\text{ kHz}$) that the physical mass of the IEM driver can't even move that quickly—it naturally "smooths" the sound out for you!
    

---

## 🧠 Questions for Further Thought

### 1. Why does this only work at high sample rates?

If your sample rate is low (e.g., $8\text{ kHz}$), the "high frequency" where we hide the noise is only $4\text{ kHz}$. That’s right in the middle of the human "screaming" frequency! Noise shaping only sounds good when the Nyquist limit is high enough to hide the garbage.

### 2. Higher Order Shaping?

We used `lastError`. If we used `(lastError * 2) - (errorBeforeLast)`, we would have a **Second-Order** shaper. This pushes the noise even more aggressively into the highs.

### 3. Is this "Cheating"?

In DSP, we call this **Spectral Shaping**. It is the ultimate expression of first-principles thinking: realizing that you don't have to solve a problem (Noise) if you can just move the problem to a place where nobody notices it.