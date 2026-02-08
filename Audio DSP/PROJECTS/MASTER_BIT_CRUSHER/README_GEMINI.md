# Master Bit Crusher

## 1. High-Level Goals (The "Why")

1. **Portfolio Value:** Move beyond "toy code" (single files) to "Systems Engineering." Show that I can architect a library, not just write a loop.
    
2. **Modularity:** Every DSP component (Quantizer, Dither, Filter) must be isolated, testable, and reusable.
    
3. **Measurability:** The engine includes its own "Lab Equipment" (Null Testers, Scopes) to mathematically prove the audio quality.
    
4. **Real-World I/O:** Must handle actual WAV/FLAC files, not just synthesized sine waves.
    

---

## 2. System Architecture

We separate the system into four distinct layers. This prevents "Spaghetti Code."

### Layer 1: The App (`src/app/`)

- **Role:** The Conductor. It parses user input, loads the config, and orchestrates the data flow. It knows _nothing_ about math.
    
- **Key Files:**
    
    - `main.cpp`: The entry point.
        
    - `Config.h`: A central struct holding all knobs (Bit Depth, Attack Time, Jitter Amount).
        

### Layer 2: The DSP Core (`src/dsp/`)

- **Role:** The Muscle. These classes process audio samples. They are small, fast, and hold their own state.
    
- **Key Files:**
    
    - `Quantizer.h`: Vertical resolution reduction (Amplitude).
        
    - `Resampler.h`: Horizontal resolution reduction (Time) + Jitter.
        
    - `Dynamics.h`: The "Brain" (Envelope Following) that modulates the other two.
        
    - `Dither.h`: Noise injection and shaping.
        
    - `FilterBank.h` & `Biquad.h`: Analog modeling (DC Block, Anti-Aliasing).
        

### Layer 3: Analysis Tools (`src/analysis/`)

- **Role:** The Lab. These classes observe the signal but do not modify it.
    
- **Key Files:**
    
    - `LinearityScope.h`: Generates data to visualize the quantization staircase.
        
    - `EnobMeter.h`: Measures quality degradation.
        
    - `GlitchDetector.h`: Flags bad discontinuities (clicks/pops).
        

### Layer 4: Infrastructure (`src/utils/`)

- **Role:** The Plumber. Handles file loading, saving, and data formatting.
    
- **Key Files:**
    
    - `AudioFile.h`: Wrapper around `dr_wav` / `dr_flac`.
        
    - `CsvWriter.h`: Exports analysis data for plotting in Excel/Python.
        

---

## 3. The Signal Pipeline (Data Flow)

This is the exact order a sample travels through the engine. This order is critical; changing it breaks the physics of the effect.

Code snippet

```
graph TD
    A[Input File] --> B[DC Blocker]
    B --> C{Envelope Follower}
    C -- Control Signal --> D[Modulation Logic]
    B --> E[Resampler / Sample & Hold]
    D -- Modulates Sample Rate --> E
    E --> F[Anti-Aliasing Filter]
    F --> G[Dither Engine]
    G --> H[Quantizer]
    D -- Modulates Bit Depth --> H
    H --> I[Output File]
```

**Why this order?**

1. **DC Block First:** Removes offset so envelope follower tracks accurately.
    
2. **Envelope Detection:** Must read the _clean_ signal dynamics, not the crushed version.
    
3. **Resampler:** Reduces sample rate (Time).
    
4. **Anti-Alias:** Must happen _before_ quantization to smooth out the time-steps, otherwise, the Quantizer will create unpredictable intermodulation.
    
5. **Dither:** Must happen immediately before Quantization to linearize the error.
    
6. **Quantizer:** The final destructive step.
    

---

## 4. Theory to Code Mapping

This table connects your learning Roadmap to the actual files.

|**Roadmap Concept**|**Theory Note**|**Implementation File**|**Key Functionality**|
|---|---|---|---|
|**Amplitude Logic**|[[01_Amplitude_Logic]]|`src/dsp/Quantizer.h`|`midTread()`, `midRise()`, Bit Reduction|
|**Quantization Error**|[[05_Quantization_Error_Analysis]]|`src/analysis/LinearityScope.h`|Visualizing the "Staircase"|
|**Dither & Noise Shaping**|[[04_Dither_Theory_Chaos_and_TPDF]]|`src/dsp/Dither.h`|TPDF Noise Generation, Error Feedback Loop|
|**IIR Filters**|[[02_IIR_Filter_Design_Leaky_Integrator]]|`src/dsp/Biquad.h`|`process()` (Direct Form I)|
|**DC Correction**|[[03_DC_Offset_Correction_Strategies]]|`src/dsp/FilterBank.h`|High Pass Filter ($R=0.995$)|
|**Anti-Aliasing**|[[01_Decimation_and_Anti_Aliasing]]|`src/dsp/FilterBank.h`|Boxcar (Moving Average) Logic|
|**Envelope Ballistics**|[[02_Envelope_Following_Ballistics]]|`src/dsp/Dynamics.h`|Recursive Filter, Attack/Release Branching|
|**Dynamics/Sidechain**|[[02_Envelope_Following_Ballistics]]|`src/app/main.cpp`|Logic mapping Envelope (0..1) to Bits (4..16)|
|**Sample & Hold**|[[04_Variable_Sample_Rate_Emulation]]|`src/dsp/Resampler.h`|Zero Order Hold, Clock Jitter|

---

## 5. Specific Implementations & Logic

### A. The "Smart" Quantizer

- **Logic:** Instead of hardcoding `bits`, it accepts a `double` variable.
    
- **Code:** `stepSize = 2.0 / pow(2, currentBits);`
    
- **Why:** Allows the LFO or Envelope to modulate "smoothly" between 4 bits and 12 bits.
    

### B. The "Jittery" Resampler

- **Logic:** Standard downsampling counts samples `1, 2, 3...` and holds.
    
- **Feature:** "Clock Jitter" adds a random offset to the counter limit.
    
- **Code:** `limit = rateDivisor + (rand() * jitterAmount);`
    
- **Why:** Simulates broken vintage samplers (SP-1200 style artifacts).
    

### C. The Logarithmic Eye

- **Logic:** The Envelope Follower tracks linear amplitude ($0.0 \dots 1.0$).
    
- **Feature:** We convert this to Decibels ($dB$) before mapping to Bit Depth.
    
- **Why:** Linear mapping feels "jumpy." Logarithmic mapping feels "musical" and matches human hearing.
    

---

## 6. Implementation Strategy (The Build Order)

Do not try to build everything at once. Follow this path:

1. **Infrastructure First:** Build `AudioFile.h`. Verify you can load a WAV and save it identically. (If this fails, nothing else matters).
    
2. **The Mono Chain:** Implement `Quantizer` and `Dither`. Hook them up in `main`. Process a file. Listen to the static crunch.
    
3. **The Time Bender:** Implement `Resampler`. Add the "Lo-Fi" sample rate reduction.
    
4. **The Brain:** Implement `Dynamics`. Wire the envelope to the bit depth. Create the "Dying Battery" effect.
    
5. **The Polish:** Add `Config` and `CsvWriter`. Export the linearity graph to prove your math works.

**Crucial Note on C++ Structure:**

Your shell script created `.h` (Header) files. For every `.h` file (e.g., `Quantizer.h`), you must create a corresponding `.cpp` file (e.g., `Quantizer.cpp`) where the actual logic lives.

- **Header (`.h`):** The menu (declarations).
    
- **Source (`.cpp`):** The cooking (implementation).
    

---

## 🏗️ Phase 1: The Foundation (Utils)

_Goal: Build the tools to generate, save, and visualize data before processing complex audio._

### 1. `src/utils/CsvWriter.cpp`

- **Goal:** A simple logger to help you debug.
    
- **Implementation:**
    
    - Use `std::ofstream`.
        
    - `writeHeader`: Loop through strings, join with commas, add `\n`.
        
    - `writeRow`: Loop through doubles, join with commas, add `\n`.
        
- **Why first?** You will use this immediately to verify your Oscillator.
    

### 2. `src/utils/Oscillator.cpp`

- **Goal:** A reliable test signal generator (so you don't always need a WAV file).
    
- **Math:**
    
    - `phaseInc_ = (twoPi * frequency) / sampleRate;`
        
    - `phase_ += phaseInc_;`
        
    - `if (phase_ >= twoPi) phase_ -= twoPi;`
        
    - **Output:** `std::sin(phase_)`.
        
- **Verification:** Create a temporary `main.cpp` that generates 100 samples of a sine wave and saves it using `CsvWriter`. Graph it in Excel. If it's a smooth wave, proceed.
    

### 3. `src/utils/AudioFile.cpp` (The Hardest Part of Phase 1)

- **Goal:** Read WAV/FLAC files using `dr_libs`.
    
- **Pre-requisite:** Download `dr_wav.h` and put it in `src/utils/`.
    
- **Implementation:**
    
    - **Include:** `#define DR_WAV_IMPLEMENTATION` inside this .cpp file (and only this one).
        
    - **Load:** Use `drwav_open_file_and_read_pcm_frames_f32`.
        
        - _Warning:_ This returns interleaved data (`L R L R`). Store it in `buffer.samples`.
            
    - **Save:** Use `drwav_open_file_and_write_pcm_frames_f32`.
        
- **Verification:** Load a song, save it as `copy.wav`. Play both. They must cancel out (Null Test).
    

---

## 🛠️ Phase 2: Signal Conditioning (Filters)

_Goal: Clean the signal before and after destruction._

### 4. `src/dsp/Biquad.cpp`

- **Goal:** A generic filter engine.
    
- **Math:** The Direct Form I difference equation.
    
    - $$y[n] = b_0 x[n] + b_1 x[n-1] + b_2 x[n-2] - a_1 y[n-1] - a_2 y[n-2]$$
        
- **Recipes:**
    
    - `makeLowPass`: Implement the RBJ Cookbook LowPass formulas (Project 16).
        
    - `makeHighPass`: Implement RBJ HighPass (needed for DC blocking).
        

### 5. `src/dsp/FilterBank.cpp`

- **Goal:** The manager that organizes the filters.
    
- **Implementation:**
    
    - `init()`: Initialize the Biquads. Set DC Blocker to HPF at 20Hz.
        
    - `processDCBlocker`: Just calls `dcBlockerBiquad.process(input)`.
        
    - `processAntiAlias`: Implement the **Boxcar** logic (Project 17).
        
        - Buffer size = `sampleRate / cutoffFreq`.
            
        - Sum samples, divide by count.
            

---

## 💥 Phase 3: The Destruction (Core DSP)

_Goal: The actual Bit Crusher logic._

### 6. `src/dsp/Resampler.cpp` (Time Logic)

- **Goal:** Reduce Sample Rate and add Jitter.
    
- **Logic:**
    
    - Check `counter_`.
        
    - **If `counter_ >= currentLimit_`:**
        
        - Update `holdValue_ = input`.
            
        - Reset `counter_ = 0`.
            
        - **Jitter Logic:** `currentLimit_ = rateDivisor + (random(-1.0, 1.0) * jitterAmount)`.
            
    - **Else:**
        
        - Increment `counter_`.
            
        - Output `holdValue_` (The frozen sample).
            
    - **Return:** `holdValue_`.
        

### 7. `src/dsp/Dither.cpp` (Spectral Logic)

- **Goal:** Add noise and shape it.
    
- **Logic:**
    
    - **Generate:** `noise = (rand() - rand()) * amplitude`. (TPDF).
        
    - **Shape:** `finalNoise = noise + (errorBuffer_ * 0.5)`. (First Order Shaping).
        
    - **Return:** `input + finalNoise`.
        
    - _Note:_ Do not update `errorBuffer_` here. That happens in the Quantizer/Main loop where the error is calculated.
        

### 8. `src/dsp/Quantizer.cpp` (Amplitude Logic)

- **Goal:** Reduce Bit Depth.
    
- **Logic:**
    
    - Calculate Step: `stepSize = 2.0 / pow(2, currentBits)`.
        
    - **Mid-Tread:** `y = round(x / step) * step`.
        
    - **Mid-Rise:** `y = (floor(x / step) + 0.5) * step`. (Optional).
        
- **Verification:** Connect Oscillator -> Quantizer (4 bits) -> CsvWriter. The sine wave should look like stairs.
    

---

## 🧠 Phase 4: The Brain (Control)

_Goal: Make the effect react to the music._

### 9. `src/dsp/Dynamics.cpp`

- **Goal:** Envelope Follower.
    
- **Logic:**
    
    - Rectify: `absInput = std::abs(input)`.
        
    - **Ballistics:**
        
        - If `absInput > env`: `env += attackCoeff * (diff)`.
            
        - Else: `env += releaseCoeff * (diff)`.
            
    - **Math:** Remember to implement the `setAttackRelease` function that converts Time (ms) to Coefficients using the `exp()` formula.
        

---

## 🔬 Phase 5: The Lab (Analysis)

_Goal: Scientific proof._

### 10. `src/analysis/NullTester.cpp`

- **Logic:** `diff = original - processed`. Return `20 * log10(abs(diff))`.
    

### 11. `src/analysis/EnobMeter.cpp`

- **Logic:** Complicated (SINAD), but for now, simple implementation:
    
    - Calculate RMS of Signal.
        
    - Calculate RMS of Error (Input - Output).
        
    - `SNR = 20 * log10(SignalRMS / ErrorRMS)`.
        
    - `ENOB = (SNR - 1.76) / 6.02`.
        

### 12. `src/analysis/LinearityScope.cpp`

- **Logic:**
    
    - Store pairs of `{input, output}` in a vector.
        
    - `saveCSV`: Dump them to a file.
        
    - _Usage:_ Feed a clean Ramp signal (0.0 to 1.0) into the system to see the perfect "Staircase" graph.
        

---

## 🎛️ Phase 6: The Glue (Application)

### 13. `src/app/main.cpp`

- **Goal:** Wire it all together.
    
- **The Loop Structure (Mental Check):**
    
    C++
    
    ```
    // 1. Setup
    Config config;
    AudioBuffer audio = AudioFile::load("drums.wav");
    
    // 2. Instantiate DSP
    Quantizer quant; 
    Resampler resamp;
    Dynamics envFollower;
    // ... init all ...
    
    // 3. Process
    for (int i = 0; i < audio.samples.size(); ++i) {
        double input = audio.samples[i];
    
        // A. Analyze (Sidechain)
        double env = envFollower.process(input);
    
        // B. Modulate Parameters
        double targetBits = config.bitDepth + (env * config.modulationDepth);
        quant.setBitDepth(targetBits);
    
        // C. Audio Path
        double out = filterBank.processDCBlocker(input);
        out = resamp.process(out);
        out = filterBank.processAntiAlias(out);
        out = dither.process(out);
        out = quant.process(out);
    
        audio.samples[i] = out;
    }
    
    // 4. Export
    AudioFile::save("processed.wav", audio);
    ```