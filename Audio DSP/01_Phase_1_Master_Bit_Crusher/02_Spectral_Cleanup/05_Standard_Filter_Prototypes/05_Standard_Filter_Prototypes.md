# Standard IIR Filter Prototypes

**Tags:** #DSP #FilterDesign #InterviewPrep #Theory

---

## 1. The Concept: The "Shape" of the Filter

You already have the **Engine** (The Biquad Class).
Now you need the **Blueprints** (The Coefficients).

When we want a Low Pass Filter, we don't just guess numbers. We choose a **Prototype**—a mathematical strategy that dictates how the filter behaves. Each prototype calculates the $a$ and $b$ coefficients differently to prioritize different things.

---

## 2. The Big Three (Interview Gold)

### A. Butterworth ("The Smooth Operator")
* **Vibe:** Maximum Flatness.
* **Behavior:** It is flat in the passband (no bumps) and rolls off smoothly.
* **The Trade-off:** The roll-off is "lazy." It's not a sharp cliff. To get a sharp cut, you need a high order (lots of Biquads).
* **Best For:** Audio bridging, master bus EQ, things that need to sound "natural."
* **Interview Answer:** "Maximally flat magnitude response in the passband. No ripples."

### B. Chebyshev ("The Aggressive Cutter")
* **Vibe:** Sharpness at a cost.
* **Behavior:** It creates a much steeper cliff than Butterworth for the same order.
* **The Trade-off:** **Ripple.**
    * *Type I:* Ripples (wobbles in volume) in the Passband (the part you hear).
    * *Type II:* Ripples in the Stopband (the part you cut).
* **Best For:** Removing very specific bad frequencies where you don't care if the "good" audio is slightly colored, as long as the "bad" audio is dead.
* **Interview Answer:** "Steeper roll-off than Butterworth, but introduces ripple in the passband or stopband."

### C. Elliptic ("The Brick Wall")
* **Vibe:** Ruthless efficiency.
* **Behavior:** The steepest possible cut for a given order. It drops like a stone.
* **The Trade-off:** Ripples *everywhere* (Passband AND Stopband) AND terrible phase distortion. It smears the time domain badly.
* **Best For:** Anti-aliasing conversion (ADC/DAC) where you must kill everything above 20kHz instantly.
* **Interview Answer:** "Steepest roll-off, but has equiripple in both bands and significant phase non-linearity."

### D. Bessel ("The Time Keeper")
* **Vibe:** Phase purity.
* **Behavior:** The magnitude curve is lazy (even slower than Butterworth), but the **Phase Delay** is linear.
* **The Result:** It preserves the *shape* of transients (snare hits, square waves). It doesn't "smear" the attack.
* **Best For:** Crossovers for subwoofers (where phase alignment is critical).
* **Interview Answer:** "Maximally flat group delay. Preserves wave shape."

---

## 3. Visual Comparison

| Filter Type | Passband Ripple? | Stopband Ripple? | Roll-off Slope | Phase Distortion |
| :--- | :--- | :--- | :--- | :--- |
| **Butterworth** | No | No | Moderate | Moderate |
| **Chebyshev I** | **Yes** | No | Steep | High |
| **Chebyshev II**| No | **Yes** | Steep | High |
| **Elliptic** | **Yes** | **Yes** | **Very Steep** | **Severe** |
| **Bessel** | No | No | Slow | **Low (Linear)** |

---

## 4. How this fits your Code
You do not write a "Chebyshev Class."
You use the **Same Biquad Class**.
You just change the numbers you feed it.

* `makeButterworthCoeffs()` -> returns `{b0, b1, b2...}`
* `makeChebyshevCoeffs()` -> returns different ` {b0, b1, b2...}`

The **Engine** (Biquad) is the same. The **Fuel** (Coefficients) changes.