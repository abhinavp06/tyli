# 🎲 Project: The Dither Engine (Linear Probability)

> [!ABSTRACT] Goal
> 
> To implement Dither, a low-level noise signal added to audio before quantization. We aim to solve the "Truncation Distortion" problem where quiet sounds (like reverb tails) are abruptly cut off by the "steps" of the bit-crusher.

---

## 🏗️ First Principle Thinking

### 1. The Resolution Floor

**First Principle:** In a digital system, any signal smaller than the smallest "step" ($1 \text{ LSB}$ or Least Significant Bit) is invisible.

- If your "Mid-Tread" step is at $0.1$ and your audio is at $0.04$, the output is **0.0**. The sound is deleted.
    

### 2. The "Stochastic" Solution

- **Concept:** Instead of letting a quiet sound die, we "vibrate" it.
    
- **Physicality:** We add a random value (Noise) to the signal _before_ we round it.
    
- **The Result:** Sometimes the noise pushes the quiet $0.04$ signal up to $0.1$, and sometimes it doesn't. Over time, the _average_ value of these jumps equals $0.04$.
    
- **The Magic:** The human ear can hear the original sound "through" the noise. We have traded **Distortion** for **Hiss**.
    

### 3. Rectangular vs. Triangular (RPDF vs. TPDF)

- **RPDF (Rectangular):** One random number. It's simple but can interact poorly with the signal.
    
- **TPDF (Triangular):** Two random numbers added together. This is the industry standard because it completely "de-correlates" the quantization error from the music.
    

---

## 🧪 The Mathematical Model

To dither a signal before a bit-crusher of $L$ levels:

1. Calculate the size of one step (the LSB): $\Delta = \frac{2}{L-1}$
    
2. Generate a random value $R$ between $-\frac{\Delta}{2}$ and $+\frac{\Delta}{2}$.
    
3. Add $R$ to your input $x$.
    
4. Pass the result $x + R$ into your **Mid-Tread Quantizer**.
    

$$y = \text{Quantize}(x + \text{Random}(-\frac{\Delta}{2}, \frac{\Delta}{2}))$$

## 🎨 Visualization Guide

### What to look for in your IEM Visualizer:

1. **Without Dither:** When you play a very quiet sine wave (e.g., `0.02 * sin(t)`) through a 4-bit crusher, the visualizer shows a **dead flat line** at zero. The wave is gone.
    
2. **With Dither:** You will see a "fuzzy" or "shimmery" line. It looks like noise, but you will notice the noise "clusters" around the shape of the original sine wave.
    
3. **The Result:** Even though the signal is "crushed," the movement of the noise preserves the _timing_ and _vibration_ of the original note.
    

---

## 🧠 Questions for Further Thought

### 1. Why don't we dither at 24-bit?

We do! But at 24-bit, the noise floor is at $-144\text{ dB}$. That is quieter than the sound of blood rushing through your own ears. Dither is most critical when moving from **32-bit float** down to **16-bit** (CD quality) or lower.

### 2. Noise Shaping: The Next Level?

Can we make the dither noise quieter? Yes. **Noise Shaping** takes the dither energy and "pushes" it into the ultra-high frequencies (above $18\text{ kHz}$) where human ears are less sensitive.

### 3. Is Dither "Analog"?

In a way, yes. Analog tape has "tape hiss" which acts as a natural dither. This is one reason why some engineers think analog sounds "warmer" and "smoother" than early digital—the noise kept the low-level signals alive!