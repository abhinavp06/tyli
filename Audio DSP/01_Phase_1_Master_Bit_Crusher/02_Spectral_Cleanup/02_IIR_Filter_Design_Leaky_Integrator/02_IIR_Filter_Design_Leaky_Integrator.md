# IIR Filter Design: The Leaky Integrator

**Tags:** #dsp #filters #iir #math #phase_1
**Related:** [[01_Noise_Shaping_and_Error_Feedback]], [[03_DC_Offset_Correction_Strategies]]

---

## 1. The Core Concept
The **Leaky Integrator** is the simplest form of an **Infinite Impulse Response (IIR)** filter. It is a **First-Order Low Pass Filter**.

* **"Integrator":** It accumulates value over time (like a bucket filling with water).
* **"Leaky":** It constantly loses value (like a hole in the bottom of the bucket).

**The Physical Analogy:** A heavy door with a hydraulic closer.
* **Input:** You pushing the door.
* **Output:** The door's actual position.
* If you push efficiently (Low Frequency), the door moves with you.
* If you vibrate your hand against it rapidly (High Frequency), the door's inertia ignores you. It effectively filters out the jitters.

## 2. The Math: The Difference Equation
Unlike a Finite Impulse Response (FIR) filter which averages *past inputs*, an IIR filter relies on *past outputs*. This creates **Feedback** and **Memory**.

$$y[n] = y[n-1] + \alpha \cdot (x[n] - y[n-1])$$

* $y[n]$: Current Output.
* $y[n-1]$: Previous Output (The "Memory").
* $x[n]$: Current Input.
* $\alpha$ (Alpha): The coefficient ($0.0$ to $1.0$).

**Interpretation:**
"The new position is the old position, plus a fraction of the distance to the new target."

## 3. The Alpha ($\alpha$) Coefficient
The behavior depends entirely on $\alpha$:
* **$\alpha = 1.0$:** Output = Input. No filtering. (Instant change).
* **$\alpha = 0.1$:** Slow response. Heavy filtering.
* **$\alpha = 0.001$:** Extremely slow drift. Used for DC tracking.

### Time Constant ($\tau$)
The time it takes for the filter to reach $\approx 63.2\%$ ($1 - 1/e$) of a step input.
$$\tau \approx \frac{1}{\alpha} \text{ (in samples)}$$

## 4. The "Denormal" Problem
In C++, floating-point numbers have a standard format (IEEE 754). When a number gets extremely close to zero (e.g., $1.0 \times 10^{-35}$), the CPU switches to "Denormal Mode" to maintain precision.
* **Consequence:** This mode is 100x slower than normal math.
* **Context:** In an IIR filter, the "tail" of the reverb/decay gets infinitely small but never truly hits zero. This can spike CPU usage to 100%.
* **Fix:** We manually snap very small numbers to `0.0f`.

## 5. Applications
1.  **Parameter Smoothing:** Removing "Zipper Noise" when a user turns a knob rapidly.
2.  **Envelope Followers:** Tracking the loudness of a signal (for Compressors).
3.  **DC Blocking:** Used as the reference "Average" to subtract from the signal.
4.  **Tone Shaping:** Taming the harsh high-end of distortion/bit-crushing.

---
