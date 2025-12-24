# 🎹 DSP Phase 1: Dither Theory & TPDF Mastery
**Status:** 🏗️ Implementation Phase
**Mentor:** Senior Audio Engineer
**Focus:** Amplitude Logic (Vertical) vs. Temporal Logic (Horizontal)

---

## 1. The Physics: The "Staircase Ghost"
Quantization is not just "losing data"; it is a non-linear transformation that creates **Quantization Distortion**.

- **The Problem:** Without dither, the error between the "analog" input and the digital "step" is **correlated** to the signal. 
- **The Result:** As a signal decays (like a reverb tail), it doesn't fade into hiss; it turns into a harsh, metallic buzzing.
- **The Solution:** Dither. By adding noise *before* quantization, we break the correlation. We trade a slightly higher noise floor for perfect linearity.



---

## 2. The Math: Triangular Probability Density Function (TPDF)
We use TPDF because it is the only distribution that eliminates **Noise Modulation** (the "breathing" hiss).

### The Logic
- **RPDF (Rectangular):** Uses one random variable. Noise power changes depending on where the signal sits on the grid.
- **TPDF (Triangular):** Created by summing two independent RPDF sources.
- **Equation:** $$y[n] = Q(x[n] + (\text{rnd}_1 - \text{rnd}_2))$$
- **Amplitude:** The dither signal must be exactly **2 LSB** (Least Significant Bits) peak-to-peak ($[-1.0, 1.0]$ in terms of the quantization step).



---

## 3. The 4 Mandatory Projects: Strategic Rationale
We divide this into four projects to isolate different failure points in the signal chain. If you mix them all at once, you won't know if your "noise" is coming from a math error or a timing error.

### Project 1: The Fast TPDF Engine
- **Strategic Goal:** Mastering **Vertical Linearity**.
- **The "Why":** Most juniors divide the random sum by 2.0. This project forces you to verify the 2 LSB peak-to-peak requirement. Without this, the noise floor "breathes" with the music, which is an immediate fail in professional plugin testing.

### Project 2: The Dither Gate ("Black Background")
- **Strategic Goal:** Mastering **Signal-to-Noise Management**.
- **The "Why":** Audio customers hate "idling hiss." This project teaches you to manage "state" (counters) and logic thresholds (`epsilon`) in a real-time environment. It ensures your plugin doesn't add noise to a silent track.

### Project 3: The Temporal Bridge (Sample-and-Hold)
- **Strategic Goal:** Mastering **Cross-Domain Interaction**.
- **The "Why":** This is where you learn that order-of-operations is king. If you dither *before* you sample-and-hold, the noise gets "smeared" horizontally, creating unpleasant pitch artifacts. This project enforces the "Horizontal before Vertical" rule.

### Project 4: Professional Architecture
- **Strategic Goal:** Mastering **Computational Efficiency**.
- **The "Why":** A plugin that sounds great but eats 15% of the CPU is useless. This project forces you to pre-calculate scales outside the sample loop, moving from "math experiment" to "production tool."
- **Requirements:**
	1. **Horizontal:** `rate_divider` with a `phase_accumulator`.
	2. **Horizontal:** A `jitter` parameter that modulates the `phase_accumulator` threshold.
	3. **Gate:** The 512-sample `silence_counter` with `epsilon` check.
	4. **Vertical:** The `TPDFDitherer` (2 LSB range, LCG-based).
	5. **Vertical:** Mid-Tread Quantization to the target `bit_depth`.
	6. **Optimization:** Pre-calculate the `inv_scale` so you aren't calling `pow()` or `1<<bit_depth` every sample.

---

## 4. The Critical Eye: Amplitude vs. Temporal
| Feature | Logic Type | Impact |
| :--- | :--- | :--- |
| **Bit Depth** | Vertical (Amplitude) | Dynamic Range / Noise Floor |
| **Sample Rate** | Horizontal (Temporal) | Frequency Response / Aliasing |
| **Dither** | Vertical Fix | Eliminates Distortion / Adds Hiss |
| **Sample-and-Hold** | Horizontal Change | Creates Aliasing (Metallic Tones) |

---

## 5. Visualization: "Rain on the Roof"
- **Quantization:** Water falling into buckets. If you don't move the hose, the splash is predictable (Distortion).
- **RPDF Dither:** Shaking the hose uniformly. The splash volume changes depending on your aim (Noise Modulation).
- **TPDF Dither:** Shaking the hose with a triangular bias. The splash volume is **perfectly constant** no matter where you aim.



---

# 🛡️ Phase 1 Concept Verification

## 1. Amplitude (Vertical)
- [ ] **Quantization Distortion:** Do I understand that rounding is a *deterministic* error correlated to the signal?
- [ ] **TPDF vs RPDF:** Do I know why summing two noise sources creates a constant noise floor power (Rain on the Roof)?
- [ ] **Mid-Tread vs Mid-Rise:** Can I visualize why Mid-Tread allows for a "Perfect Silence" at 0V?
- [ ] **LSB Scaling:** Do I understand why dither amplitude MUST be exactly 2 LSB peak-to-peak?

## 2. Temporal (Horizontal)
- [ ] **Sample-and-Hold:** Can I visualize how holding a value creates "staircase" artifacts in time?
- [ ] **Jitter:** Do I understand that jitter is "time-domain noise" and it affects high frequencies the most?
- [ ] **Aliasing:** Do I understand that frequencies above Nyquist ($F_s/2$) fold back into the audible spectrum?

## 3. Order of Operations
- [ ] **Horizontal -> Vertical:** Why must we capture time before we fix the amplitude?
- [ ] **Real-Time Safety:** Why are `std::pow` and `new` forbidden in the `process_sample` loop?

# Some Questions

### Question 1: The Spectral Geometry of the "Chirp"
You correctly noted that dithering before the sample-and-hold causes "chirping birds."
The Question: In terms of the frequency spectrum, why does holding a random number for $N$ samples change it from "white noise" (hiss) into a sound with distinct, pitched artifacts (chirps)?
----> The idea of dithering is to have a constant shake which ends up representing a near identical analogue wave. If we sample and hold after dithering, the dither that we have added will be frozen. The reason it sounds like chirps is because the noise is now being elongated compared to being random for every sample before (like how white noise is supposed to be).

### Question 2: The Logic of the Gate Epsilon
In your code, you use 1e-7f as your threshold for the silence_counter.
The Question: Why can't we just use if (hold_value == 0.0f)? What specific real-world scenario would cause that "perfect zero" check to fail, keeping the dither engine running forever?
----> There are multiple reasons for using 1e-7f instead of 0.0f. One is that we don't want to end up dividing by 0 in our dsp logic (if we ever encounter this division). Second is that floating point numbers very close to zero can cause significant slowdows on certain CPU and DSP architectures. Another is that there might be some values which should theoretically be zero but might be slightly different due to inaccuracies present in floating points. The biggest reason in high-end audio is **Residual Noise**. In a real signal chain, the "silence" coming from a previous plugin is almost never a perfect zero; it's usually at $-120\text{dB}$ or has a tiny bit of DC offset. If you use `0.0f`, your dither engine will never sleep, and your noise floor will stack up across 50 tracks until the user's speakers are hissing like a pit of snakes.

### Question 3: The Linear Averaging Paradox
Your code dithers the hold_value every sample, even when hold_value is constant.
The Question: If the signal is sitting at exactly 2.5 and your quantization steps are at 2.0 and 3.0, describe what the output samples look like over time and why the human brain perceives this as "2.5" instead of just a noisy error.
----> The Analogy: The Rapidly Flickering Light.
Imagine a lightbulb that can only be Fully On (3.0) or Fully Off (2.0). If I want the room to be exactly half-bright ($2.5$), and I flick that switch back and forth 44,100 times a second, what do you see? You don't see a flashing strobe; you see a steady, half-brightness light.
**In your code:**
- Sample 1: $2.0$
- Sample 2: $3.0$
- Sample 3: $2.0$
- Sample 4: $3.0$
- **The Brain's Result:** Your ear acts as a **Low-Pass Filter**. It integrates those high-speed jumps and calculates the **Average (Mean)**.
By dithering every sample, you ensure that over a small window of time, the _average_ value of your "vibrating steps" perfectly matches the original analog signal. This is how 16-bit audio can represent sounds at $-110\text{dB}$, even though the math says the noise floor should be at $-96\text{dB}$
