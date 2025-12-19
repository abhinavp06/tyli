# 🗜️ Project: The Quantizer Architect (Mid-Rise vs. Mid-Tread)

> [!ABSTRACT] Goal
> 
> To implement and visualize the two primary philosophies of quantization. We are moving beyond simple "rounding" and choosing exactly where the "steps" of our digital ladder sit relative to the zero-line (silence).

---

## 🏗️ First Principle Thinking

### 1. The Symmetry Problem

**First Principle:** Binary numbers are even ($2^n$), but a balanced audio wave is odd (Positive, Negative, and **Zero**).

- If you have 4 levels (2-bit), you cannot have a symmetrical wave _and_ a perfect zero. You must choose to prioritize the "Step" or the "Landing."
    

### 2. Mid-Tread (The "Silent" Path)

- **Concept:** The "Zero" is a flat level (a "Tread").
    
- **Physicality:** When the input is very quiet, the output stays at **exactly 0.0**.
    
- **The Result:** Good for noise reduction in silence, but you "lose" tiny details that aren't strong enough to jump to the first step.
    
- **Visual:** The center of the graph is a horizontal line.
    

### 3. Mid-Rise (The "Active" Path)

- **Concept:** The "Zero" is a transition point (a "Rise").
    
- **Physicality:** The system _cannot_ represent 0.0. Even at silence, it must choose between a tiny positive value ($+0.1$) or a tiny negative value ($-0.1$).
    
- **The Result:** The speaker is always "jittering" or biased. This creates a constant low-level hiss or "square wave" even during silence.
    
- **Visual:** The center of the graph is a vertical jump.
    

---

## 🧪 The Mathematical Models

### Mid-Tread Formula

To force a "dead-zone" at zero:

$$y = \text{floor}(x \cdot \frac{L-1}{2} + 0.5) \cdot \frac{2}{L-1}$$

(Where $L$ is the number of levels, usually an odd number to be effective).

### Mid-Rise Formula

To force a "jump" at zero:

$$y = (\text{floor}(x \cdot \frac{L}{2}) + 0.5) \cdot \frac{2}{L}$$

|**Feature**|**Mid-Tread**|**Mid-Rise**|
|---|---|---|
|**At Zero Crossing**|You will see a flat BLUE line (`-`) across the center.|The wave will "teleport" from negative to positive instantly (RED `\|`).|
|**Low Amplitude**|The wave disappears (Gate effect).|The wave becomes a small Square Wave.|
|**Physics Failure**|Minimal at silence.|Constant "Infinite Force" snap at the zero-line.|
## 🧠 Questions for Further Thought

### 1. Why do "Dither" noises exist?

In **Mid-Tread** systems, very quiet sounds (like the tail of a reverb) simply get cut off when they fall below the first "step." Engineers add a tiny bit of random noise (**Dither**) to "kick" those quiet sounds back and forth over the step so we can still hear them.

### 2. Which is better for IEMs?

Most modern high-end DACs aim for a Mid-Tread style or extremely high bit-depths where the steps are so small the distinction is irrelevant. However, in low-power DSP, **Mid-Rise** is sometimes easier to calculate because it maps perfectly to 2's complement binary.

### 3. How does this link to "DC Offset"?

If your **Mid-Rise** quantizer isn't perfectly balanced, your speaker cone might stay "pushed out" even when no music is playing. This is **DC Offset**, and it can heat up the voice coils in your drivers!