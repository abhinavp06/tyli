---
tags:
  - "#SampleRate"
  - "#BufferSize"
  - "#Latency"
Related Notes: "[[02. Sample Rate, Buffer Size and Latency#^9f5fe9]]"
---
# Idea
The base idea for this implementation is to simulate a sine wave and visualize a "glitch" or "pop" based on my note [[02. Sample Rate, Buffer Size and Latency#^9f5fe9]]. We want to see the "physics" of the speaker cone moving.

# First Principle Thinking
## Phase 1: The Signal (Motion)
**First Principle:** Sound is just a sequence of positions over time.
1.  **Motion:** If I want a wave, I need a value that oscillates. The simplest oscillation in nature is the circle (Sine Wave).
2.  **State:** Computers are discrete. To know where the "circle" is right now, I must store a **Phase** variable.
3.  **The Glitch Rule:** A glitch is a **violation of continuity**. Normally, $Position_{now}$ must be very close to $Position_{last}$. To simulate a glitch, I must create a logic branch that says: *"Ignore the circle; set position to zero immediately."*

## Phase 2: The History (Time)
**First Principle:** To see a graph, we must know the past.
If I only have the "current" sample, I have a dot. To have a line, I need a history.
- **The Constraint:** My screen has a fixed width ($W$). I only care about the last $W$ samples.
- **The Choice:** I need a "First-In, First-Out" (FIFO) structure.
    - *Reasoning:* When a new sample arrives, the oldest one is no longer relevant to the screen. A `std::deque` (Sliding Window) is the natural mathematical fit.

## Phase 3: The Mapping (Space)
**First Principle:** The math space is not the screen space.
- **Audio World:** Values go from $-1.0$ to $+1.0$.
- **Terminal World:** Rows go from $0$ to $30$. Columns go from $0$ to $100$.
We need a "Mapping Function":
- If the audio is $1.0$, it should draw on Row $0$.
- If the audio is $-1.0$, it should draw on Row $30$.
- *Formula:* $Row = \text{round}\left( (1.0 - \text{Amplitude}) \times \frac{\text{TotalRows}}{2} \right)$.

## Phase 4: The Renderer (The O(n) Optimization)
**First Principle:** Don't ask questions you know the answer to.
* **The Naive Approach ($O(N^2)$ Scanline):** Loop through every pixel on screen ($30 \times 100 = 3000$ checks) and ask "Is there a dot here?". Most answers are "No." This is slow.
* **The Efficient Approach ($O(N)$ Canvas):** Treat the screen as a **stack of paper strips**.
    1.  Create a blank canvas.
    2.  Iterate through the audio buffer **once**.
    3.  Calculate the exact address (`Row`, `Col`) and "stamp" the character `*` directly into memory.
    4.  **Result:** 30x faster and computationally distinct from the printing process.

## Phase 5: The Glitch (The Vertical Connector)
**First Principle:** A digital discrete sample hides the path between points.
If sample A is at Row 10 and sample B is at Row 20, the computer sees two dots. The *physics* sees a violent snap (Infinite Acceleration).
* **The Logic:** We track `previousRow`.
* **The Threshold:** If `distance > 2`, the slew rate is too high.
* **The Visual:** We loop from `min` to `max` and fill the vertical gap with `|`. This draws the "invisible force" of the glitch.
---
# Implementation Steps

## Step 1: The Coordinate Mapper
We need a translation layer that safely converts a float (Audio) to an int (Array Index). It must handle clamping so we don't crash by trying to draw outside the vector.

```cpp
int coordinate_mapper(float value) {
    if (value > 1.0f) value = 1.0f; // Safety Clamp
    if (value < -1.0f) value = -1.0f;
    // Map 1.0 -> 0 and -1.0 -> TOTAL_ROWS
    int result = std::round((1.0f - value) * (TOTAL_ROWS / 2));
    return result;
}
```

## Step 2: The Canvas Strategy ($O(n)$ Setup)
Instead of printing immediately, we allocate a "Frame Buffer" in memory. This allows us to modify pixels in any order before showing them to the user. We initialize the baseline (`-`) here to save processing time later.
```cpp
// Create a vector of strings (Rows), initialized with spaces
std::vector<std::string> canvas(TOTAL_ROWS, std::string(buffer.size(), ' '));
canvas[ZERO_AUDIO_ROW] = std::string(buffer.size(), '-'); // Draw the "Ground"
```

## Step 3: The Physics Loop
We iterate through the buffer exactly once. Inside this loop, we perform two tasks:
1. **Glitch Detection:** Compare `targetRow` with `previousRow`. If the gap is huge, fill it with `|`.
2. **Signal Stamping:** Place the `*` at the current sample's position.

```cpp
for (int i = 0; i < buffer.size(); i++) {
    int targetRow = coordinate_mapper(buffer[i]);

    // Glitch Connector Logic
    if (previousRow != -1 && abs(targetRow - previousRow) > GLITCH_THRESHOLD) {
        // Fill the gap with pipes '|'
    }
    
    canvas[targetRow][i] = '*'; 
    previousRow = targetRow;
}
```

## Step 4: JIT (Just-In-Time) Coloring
We cannot store ANSI color codes inside the std::string canvas because they are "invisible" characters that mess up the grid alignment.

Solution: Store plain symbols in the canvas, and apply color only during the final cout.
- `|` becomes **RED** (Danger/Glitch)
- `*` becomes **GREEN** (Signal)
- `-` becomes **BLUE** (Baseline)
- 
## Step 5: The Sliding Window

To create the "Oscilloscope" movement, we use a `std::deque`.
- **Pop Front:** Remove the oldest sample (Time $t-100$).
- **Push Back:** Add the newest sample (Time $t$).
- **Convert:** Copy the deque to a `std::vector` to pass it to our renderer.


# Questions
### Why is the terminal space inverted?
The terminal is inverted because it was designed to mimic a printing press, not a math graph. We start at the top because that's where the page starts. This is also why the `TOP_ROW` constant is 0 and the `BOTTOM_ROW` constant is 29 (since indexes start from 0).

### Wait, so is the canvas a 2D matrix?
Yes. Although we declare it as `std::vector<std::string>`, logically it is a 2D Grid `[Row][Col]`.
- **Rows (Y-axis):** The vector indices.
- Columns (X-axis): The characters inside the string. This structure allows us to access ```canvas[targetRow][i]``` instantly ($O(1)$) without searching, which is the key to the speed of this algorithm.
    
### How do I decide a THRESHOLD for the glitch?
The threshold is a balance between **Physics** (DSP) and **Aesthetics** (Visuals).
1. **DSP Physics:** A 50% instant jump (amplitude change of 1.0) is almost certainly a "pop."
2. **Visuals:** Since our screen resolution is low (30 rows), a tiny movement in audio might look like a 2-row jump.
    - _Recommendation:_ Set `GLITCH_THRESHOLD = 2`. This ensures we don't draw red lines for normal fast signals (like a snare drum), but we DO draw them for unnatural snaps (like a buffer underrun).

### What would a Production Grade IEM use? 
While this C++ code simulates the logic perfectly, a real firmware implementation for an IEM (like the Letshuoer S12 Pro DSP cable) would differ in three key ways:
1. **Memory Management (Static vs. Dynamic):**
    - _This Code:_ Uses `std::vector` and `std::deque`. These ask the OS for memory every frame (`malloc`). This is non-deterministic and can cause audio dropouts.
    - _IEM Firmware:_ Uses **Static Arrays** (e.g., `float buffer[128]`) allocated at compile-time. We never create or destroy memory while music is playing.
2. **The Buffer Structure (Ring vs. Deque):**
    - _This Code:_ `std::deque` physically moves data in memory.
    - _IEM Firmware:_ Uses a **Ring Buffer (Circular Buffer)**. We don't move the data; we just move a "Write Head" pointer around the circle.
3. **The Glitch Handling (Limiter vs. Visualizer):**
    - _This Code:_ Visualizes the glitch with a red line `|`.
    - _IEM Firmware:_ Uses a **Look-Ahead Limiter**. If it detects the Slew Rate will exceed the threshold, it mathematically "bends" the wave down to prevent the speaker from snapping. This protects the Planar Magnetic drivers from physical damage.