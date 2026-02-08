# Noise Shaping and Error Feedback

**Tags:** #dsp #quantization #dither #noise_shaping #phase_1
**Related:** [[04_Dither_Theory_Chaos_and_TPDF]], [[02_IIR_Filter_Design_Leaky_Integrator]]

---

## 1. The Core Concept
Standard Dither (TPDF) linearizes quantization error, removing harmonic distortion, but it leaves a **flat white noise floor**. In low-bit scenarios (8-bit, 12-bit), this hiss is audible.

**Noise Shaping** is a technique to alter the spectral content of this error. We cannot reduce the *total* energy of the error (conservation of energy), but we can redistribute it.
* **Goal:** Push quantization noise power away from sensitive frequencies (1kHz - 4kHz) and into the high frequencies (>15kHz) where the human ear is less sensitive.
* **Analogy:** Sweeping dirt (noise) from the center of the room (mids) to the walls (highs).

## 2. The Architecture: Error Feedback Structure (EFS)
The most common implementation is the **Error Feedback Loop**. We measure the error introduced by the quantizer in the *current* sample and subtract it from the *next* sample.

### The Flow
1. **Input ($x[n]$):** High-precision signal.
2. **Error Feed ($e[n-1]$):** The error from the previous step.
3. **Compensated Input ($u[n]$):** $x[n] - e[n-1]$.
4. **Quantizer ($Q$):** Rounds $u[n]$ to the nearest bit step.
5. **Output ($y[n]$):** The result.
6. **New Error ($e[n]$):** Difference between Output and Compensated Input.

$$e[n] = y[n] - u[n]$$

## 3. The Math (First-Order)

### Time Domain
$$u[n] = x[n] - e[n-1]$$
$$y[n] = Q(u[n])$$

If we expand the output in terms of error:
$$y[n] = x[n] + e[n] - e[n-1]$$

* $x[n]$ is our desired signal.
* $e[n] - e[n-1]$ is the noise.

### Frequency Domain (Z-Transform)
The noise term corresponds to a filter transfer function:
$$H_e(z) = 1 - z^{-1}$$
This is a **High Pass Filter**.
* At DC (0 Hz, $z=1$): Gain is $1 - 1 = 0$. (Noise is suppressed).
* At Nyquist (Fs/2, $z=-1$): Gain is $1 - (-1) = 2$. (Noise is doubled/boosted).

This confirms the "Spectral Tilt." We silence the lows and boost the highs.

## 4. Implementation (C++20)

**Key Constraint:** This must be done *after* dither is added, but *before* final quantization.

```cpp
// 1. Calculate Quantization Steps
float steps = pow(2.0f, bitDepth) - 1.0f;

// 2. Error Feedback (Subtract previous error)
float compensated = input_sample - m_prev_error;

// 3. Quantize
float scaled = compensated * steps;
float quantized_scaled = std::round(scaled);
float output = quantized_scaled / steps;

// 4. Store New Error
// CRITICAL: Error is difference between Output and Compensated Input
m_prev_error = output - compensated;
```

## 5. Visual Analysis (FFT)

When observing a sine wave (e.g., 100Hz) on a spectrum analyzer:

|**Feature**|**Flat TPDF**|**Noise Shaped**|
|---|---|---|
|**Signal Peak**|Sharp spike at 100Hz|Sharp spike at 100Hz|
|**Noise Floor**|Flat, horizontal line|Sloped line (Ramp) /|
|**Low Freq Noise**|Moderate|Very Low (Suppressed)|
|**High Freq Noise**|Moderate|Very High (Boosted)|

## 6. Stability and Limits

- **First Order:** Always stable.
    
- **Higher Orders:** Can become unstable if coefficients are not chosen carefully (Lipshitz & Vanderkooy criteria). If the error feedback loop gain exceeds unity for too long, the values explode.

### **Component: First-Order Noise Shaper**

**File:** `implementations/01_first_order_noise_shaper.h`

**Type:** Class / State Machine

**Purpose:** Implementation of a standard Error Feedback Structure (EFS) to push quantization noise into higher frequencies.

#### **1. Member Variables (State)**

C++

```
private:
    float m_prev_error = 0.0f;
```

- **What it is:** The memory slot. This holds the value $e[n-1]$.
    
- **Why we need it:** Noise shaping is a **recursive** process. We cannot shape the current sample without knowing how badly we "messed up" the previous sample.
    
- **Initialization:** Starts at `0.0f` (silence).
    

#### **2. The Reset Function**

C++

```
public:
    void reset() {
        m_prev_error = 0.0f;
    }
```

- **The Logic:** When the DAW transport stops or you insert the plugin on a new track, the history is invalid.
    
- **The Risk:** If you don't reset, you might have a "stale" error value from a loud song applied to the start of a quiet song, causing a click. Always clear your buffers.
    

#### **3. The Process Function**

This is the hot path. It runs 44,100+ times per second.

**Step A: Define the Grid**

C++

```
const float steps = std::pow(2.0f, bit_depth) - 1.0f;
```

- Calculates the resolution of our target "grid."
    
- _Example:_ 4 bits = 15 steps. We are dividing the amplitude range -1.0 to 1.0 into 15 distinct slots.
    

**Step B: The Feedback (The "Shaping")**

C++

```
float compensated_input = input_sample - m_prev_error;
```

- **The Magic:** We subtract the previous error.
    
- **Why Subtract?** This creates **negative feedback**. If the previous sample was rounded _down_ (error was positive), we push this sample _up_ to compensate. This rapid correction creates the high-frequency oscillation (blue noise).
    

**Step C: The Quantization (The "Crushing")**

C++

```
float scaled = compensated_input * steps;
float quantized_scaled = std::round(scaled);
float output_sample = quantized_scaled / steps;
```

- **Standard Mid-Tread Quantization:**
    
    1. Expand the range (0.5 becomes 7.5).
        
    2. Round to integer (7.5 becomes 8.0).
        
    3. Shrink back to normalized range (8.0 becomes 0.533...).
        

**Step D: Error Calculation (The "Accounting")**

C++

```
m_prev_error = output_sample - compensated_input;
```

- **The Trap:** Many juniors calculate `output_sample - input_sample`. This is **WRONG**.
    
- **The Law:** You must calculate error relative to the signal _entering the quantizer_ (`compensated_input`).
    
- **Logic:** The "Quantizer" doesn't know about the original signal. It only knows what it was handed. The error is the difference between what it _was given_ and what it _produced_.

**CRITICAL:** The effectiveness of Noise Shaping depends on the loop gain. In this First-Order implementation, the coefficient is effectively `-1.0` (implied). If we were to change the subtraction line to `input_sample - (0.5 * m_prev_error)`, we would change the "slope" of the noise floor. The code above uses full feedback for maximum shaping stability.