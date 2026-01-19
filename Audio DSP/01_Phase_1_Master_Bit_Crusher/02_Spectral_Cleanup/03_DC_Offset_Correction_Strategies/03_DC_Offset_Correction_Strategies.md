# DC Offset Correction Strategies

**Tags:** #DSP #AudioProgramming #Filters #Cpp #SpectralCleanup

---

## 1. The Physics: "The Invisible Wind"

**The Analogy:**
Imagine an audio signal as a flag flapping in the wind.
- **AC (Sound):** The flapping motion. This is the audio information.
- **DC (Offset):** A strong, steady wind blowing constantly in one direction.

If the "DC Wind" is strong, the flag doesn't flap around the pole; it flaps while being held out to the side.

**In the Speaker Cone:**
- **Normal:** The cone vibrates symmetrically around its resting position (0.0).
- **DC Offset:** The cone is pushed out and *held* there, vibrating around a stressed position (e.g., +0.5).

**Why is this fatal?**
1.  **Headroom Theft:** If your signal is shifted by +0.2, you clip at +0.8 instead of +1.0. You lose 20% of your dynamic range.
2.  **Heat:** The speaker coil is constantly energized just to hold that position, turning electricity into heat, not sound.
3.  **Clicks:** Muting or stopping playback causes a massive "pop" as the speaker snaps back to 0.

![[dc_offset_illustration.png]]
*(Note: Paste a screenshot of an off-center waveform here)*

---

## 2. The Math: The Recursive DC Blocker

To remove DC (0 Hz), we need a **High Pass Filter** with an extremely low cutoff (10Hz - 20Hz).

### The Transfer Function (Z-Domain)
$$H(z) = \frac{1 - z^{-1}}{1 - R \cdot z^{-1}}$$

### The Difference Equation (Time-Domain)
This is what we implement in code.
$$y[n] = x[n] - x[n-1] + R \cdot y[n-1]$$

Where:
- $x[n]$ = Current Input
- $x[n-1]$ = Previous Input (Differentiator)
- $y[n-1]$ = Previous Output (Integrator)
- $R$ = Pole location (usually 0.995 to 0.999)

**The Logic:**
- $x[n] - x[n-1]$: Calculates the *change* in signal. DC does not change, so this term becomes 0 for DC.
- $+ R \cdot y[n-1]$: The "leaky" integration that maintains low-frequency AC energy.

---

## 3. Signal Flow Graph (Mermaid)

```mermaid
graph LR
    input((x[n])) --> sum1(+)
    sum1 --> output((y[n]))
    
    input -- z^-1 --> delayX[x n-1]
    delayX -- "-1" --> sum1
    
    output -- z^-1 --> delayY[y n-1]
    delayY -- "R (0.995)" --> sum1

```

## 4. Implementation (C++20)

**Class Structure:** `DCBlocker` **File:** `implementations/02_dc_blocker.h`
```

#pragma once
#include <cmath>
#include <numbers>

class DCBlocker {
public:
    DCBlocker() = default;

    /**
     * @brief Initialize the DC Blocker
     * @param sampleRate The system sample rate
     * @param cutoffFreq Corner frequency (default 20Hz). Below this, audio is attenuated.
     */
    void init(double sampleRate, double cutoffFreq = 20.0) {
        // R = exp(-2pi * fc / fs)
        // Controls how "tight" the filter is. Closer to 1.0 = Lower cutoff.
        m_R = std::exp(-2.0 * std::numbers::pi * cutoffFreq / sampleRate);
        reset();
    }

    void reset() {
        m_prevX = 0.0;
        m_prevY = 0.0;
    }

    // Real-Time Safe Process Loop
    [[nodiscard]] double process(double x) noexcept {
        // y[n] = x[n] - x[n-1] + R * y[n-1]
        double y = x - m_prevX + (m_R * m_prevY);

        // Update state
        m_prevX = x;
        m_prevY = y;

        return y;
    }

private:
    double m_R = 0.995; 
    double m_prevX = 0.0;
    double m_prevY = 0.0;
};
```

## 5. Visualization: What to Look For

### The "Square Wave Sag" (Droop)

If you feed a low-frequency square wave (e.g., 50Hz) through this filter, it will not remain square.

- **Input:** Flat tops and bottoms.
- **Output:** The flat tops will curve (decay) towards zero.
- **Analysis:** This "sag" is the filter trying to kill the "DC" component of the sustained high voltage. **This is normal and desired behavior.**

### The Linearity Check

When analyzing your **Bit Crusher**:

1. **Before Correction:** The waveform might center around -0.1 due to `floor()` functions.
2. **After Correction:** The waveform must oscillate perfectly around 0.0.

---

## 6. Critical Misconceptions

### "Why not just calculate the average and subtract it?"

**The Block Average Fallacy:**

$$ \text{Offset} = \frac{1}{N} \sum x[i] $$

$$ y[n] = x[n] - \text{Offset} $$

**Why this fails:**

1. **Latency:** You have to wait for the entire buffer ($N$ samples) to finish before you know the average.
    
2. **Non-Realtime:** You cannot do this on a live stream (microphone input) without adding delay.
    
3. **Moving Targets:** DC offset can _drift_ over time. A static subtraction doesn't adapt. The recursive filter adapts instantly.
    

### "Does this affect Bass?"

Yes.

- If $R$ is too low (e.g., 0.8), you will kill the sub-bass (kick drums will sound thin).
    
- **Rule of Thumb:** Keep cutoff below 20Hz. $R \approx 0.995$ for 44.1kHz.