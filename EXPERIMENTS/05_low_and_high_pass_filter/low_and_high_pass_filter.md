This is the "Bread and Butter" of audio engineering. Whether you are smoothing out the jagged edges of a bit-crusher or removing the rumble from a microphone, filtering is the most fundamental tool in your kit.

Here is your **Obsidian-optimized** project document for **Low-Pass and High-Pass Filters**.

---

# 🌊 Project: Linear Filters (The Frequency Gate)

> [!ABSTRACT] Goal
> 
> To implement First-Order (6dB/octave) Low-Pass and High-Pass filters. We aim to understand how the simple act of "remembering" the previous sample allows us to manipulate the frequency spectrum—specifically to "smooth out" the harsh staircase artifacts created by our bit-crusher.

---

# 🧠 First Principle Thinking

## 1. The Inertia of Sound

**First Principle:** High frequencies are fast changes; Low frequencies are slow changes.

- To remove high frequencies, we must introduce **Inertia**.
    
- **The Logic:** If we don't allow a sample to change "too fast" from the previous one, the fast vibrations (Highs) are suppressed, while the slow movements (Lows) pass through.
    

## 2. The Weighted Average

**First Principle:** A filter is just an "opinionated" average.

- If the current output is $90\%$ of the previous output plus $10\%$ of the new input, the signal cannot change instantly. It takes time to "drift" to the new value.
    
- This "drift" time is what defines your **Cutoff Frequency**.
    

## 3. The "Phase" Tax

**First Principle:** You cannot change the amplitude without changing the time.

- Because a filter relies on "memory" (the previous sample), the output is always slightly delayed compared to the input. This is called **Phase Shift**. In DSP, "Perfect Filtering" with "Zero Delay" is mathematically impossible in real-time.
    

---

# 📉 Difficulty & Scope

> [!IMPORTANT] Level: Intermediate
> 
> While the code is short, the math behind it (Z-Transforms) is deep.

- **Math Intensity:** 6/10 (Algebra and basic Trigonometry).
    
- **C++ Complexity:** 4/10 (Requires maintaining "State" between samples).
    
- **Conceptual Depth:** 8/10 (Understanding the "Complex Plane" is the key to mastery).
    

---

# 📚 Core Competencies (Prerequisites)

### 1. The Difference Equation

You must understand how to read a DSP "Map."

- **Must Learn:** $y[n] = b_0 x[n] + a_1 y[n-1]$. This is the DNA of an **IIR (Infinite Impulse Response)** filter.
    
- **Must Learn:** The difference between **Feed-Forward** (FIR) and **Feed-Back** (IIR).
    

### 2. The Z-Transform

This is the Frequency Domain's version of the "Map."

- **Must Learn:** What a **Pole** and a **Zero** are.
    
    - A **Pole** "pushes" the frequency response up (Resonance).
        
    - A **Zero** "pulls" the frequency response down (Cutting).
        

### 3. Coefficient Calculation

You can't just pick random numbers for your average.

- **Must Learn:** How to convert a frequency in Hertz (e.g., $500\text{ Hz}$) into a coefficient ($\alpha$) based on your **Sample Rate**.
    

---

# 🧩 Hints & Heuristics (Q&A)

### Q: What is the simplest way to make a Low-Pass Filter?

**Hint:** Look at the concept of **Leaky Integration**. If you take your current input and add a large portion of your _previous output_, you are creating a "leaky bucket." The fast "splashes" (Highs) are averaged out, but the "water level" (Lows) moves up and down slowly.

### Q: How do I turn a Low-Pass into a High-Pass?

**Hint:** Think about **Phase Inversion**. If you have the original "Full" signal and you subtract the "Low-Pass" version from it, what is left over? Only the "Highs." Mathematically: $\text{HighPass} = \text{Original} - \text{LowPass}$.

### Q: Why does my filter "explode" (output goes to infinity)?

**Hint:** This is the danger of **Feedback**. In an IIR filter, if your coefficient is $\ge 1.0$, the "memory" grows every sample until it hits the numerical ceiling. This is an **Unstable Pole**. Your coefficients must always stay within the "Unit Circle."

### Q: How do I make the filter "sharper"?

**Hint:** A first-order filter only has one "memory cell" ($y[n-1]$), giving you a gentle $6\text{dB}$ per octave slope. To get a steeper "brick wall" cut, you must **Cascade** filters. Run the output of Filter A into the input of Filter B. Search for: **"Biquad Filters"**.

---

# 🚀 Implementation Strategy

## The "State" Consideration

Unlike the Bit-Crusher (which is mostly stateless), a filter **must** remember the past.

- **C++ Tip:** Use a `private` member variable like `lastOutput` or `x1`, `y1`.
    
- **C++ Tip:** If you have a stereo signal (Left/Right), you need **two** separate filter instances. Never share the "memory" between two different channels, or the audio will "leak" across the stereo image!
    

## The Cutoff Math

To calculate the coefficient ($\alpha$) for a simple One-Pole filter:

$$\alpha = e^{-2\pi f / s}$$

(Where $f$ is cutoff frequency and $s$ is sample rate).

---

> [!TIP] Obsidian Next Steps
> 
> - Use `#term_IIR` and `#term_Cutoff` to link this to your **Bit-Crusher** note.
>     
> - **Note for the Visualizer:** When you apply a Low-Pass filter to your Bit-Crusher, the RED `|` bars in your visualizer will start to disappear. You are literally "sanding down" the jagged edges of the digital staircase.
>