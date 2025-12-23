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

## 🛑 Code Challenge Checklist
- [ ] Implement `fast_random()` (Xorshift/LCG).
- [ ] Implement `silence_counter` logic.
- [ ] Implement `phase_accumulator` for Sample-and-Hold.
- [ ] Correct TPDF range (Sum of two RPDFs, no division).
- [ ] Pre-calculate `invScale` for quantization.