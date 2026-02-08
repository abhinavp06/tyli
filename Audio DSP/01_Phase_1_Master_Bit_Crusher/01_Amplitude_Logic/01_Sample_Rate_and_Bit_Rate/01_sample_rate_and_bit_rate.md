# Introduction
With audio, our main goal is to try to capture all information possible of an analogue wave and convert it to digital information so that our hardware can process it. It is an approximation of reality. We attempt to capture a continuous, fluid phenomenon (sound pressure) by forcing it into a rigid 2D grid:
1. X-Axis -> 
	1. This represents the time.
	2. Data is sliced into snapshots called #term_SampleRate
2. Y-Axis ->
	1. This represents the #term_Amplitude
	2. The y-axis is sliced into measurement levels called #term_BitDepth

# Sample Rate
#definition_term_SampleRate -> The number of snapshots taken per second to measure air pressure.
- Standard: 44.1KHz or 48Khz
- Time is captured in discrete "ticks" or "samples"
- #definition_term_NyquistLimit -> The sample rate determines the #term_FrequencyBandwidth . You can only capture frequencies up to the half the #term_SampleRate  ($F_s / 2$). This is the #term_NyquistLimit

> [!QUESTION] Does higher sample rate mean "more detail"?
> **Partially.** In photography, more pixels = sharper image. In audio, 44.1kHz is mathematically sufficient to capture **everything** a human can hear (20Hz - 20kHz) due to the **Nyquist-Shannon Sampling Theorem**. 
> 
> Going higher (e.g., 192kHz) doesn't make the audible range "crisper"; it simply allows you to record ultrasonics (sound bats hear). However, higher sample rates *do* push the **Aliasing** fold-over point further away, which is useful for heavy processing like saturation.

# Bit Depth
The resolution of the "ruler" - basically the number of points present on the y-axis (both negative + positive + 0th base line)
- This determines the #term_DynamicRange ( #term_SNR )
- The limit: Low bit depth results in #term_QuantizationNoise (a grainy, fuzzy texture)
	- 16-bit = 65,536 levels (CD Quality)
	- 24-bit = 16.7 million levels (Studio Standard)
The way we arrange these levels around the 0th baseline determines the architecture ( #term_MidTread vs #term_MidRise).

# Audio Lifecycle
Here is a basic flow of how audio from an instrument is captured and the finally played out to the speakers ->
1. The "recorder" first records an analogue wave
2. This wave is converted to digital audio buffers by an #term_ADC (Analog-to-Digital converter)
3. The hardware has a fixed buffer size which defines the total amount of "samples" it can process at once
4. Once this buffer is full, a signal is sent to the CPU to start the "processing".
5. The CPU then calls a callback function of the programs we write and returns the altered information back to the buffer.
6. This buffer is then played out to the speakers by a #term_DAC (Digital-to-Analog converter)

> [!CONVERSATION BETWEEN THE HARDWARE AND CPU]
> The hardware says: _"Here is a pointer to a block of 128 floats. Fix them and give them back before I finish playing this other block."_

> [!ANALOGY]
> Think of a buffer like a **Locked Room**.
> >1. **The CPU enters the room** (The Callback). The door is locked behind it. The speaker **cannot see** what is happening inside.
>> 2. **The CPU processes the whole block.** It loops from index `0` to `127`. It changes all the numbers.
>>3. **The CPU leaves the room.** Only _after_ the CPU is finished and the function returns does the hardware "unlock" that memory.
>>4. **The Speaker enters the room.** It now reads the _entirely finished_ block at its own steady pace.
>
>In high-end audio hardware (DMA - Direct Memory Access), the speaker doesn't "move"—the **Pointers** move.
 > >Imagine two rooms: **Room A** and **Room B**.
 > > 1. **Phase 1:** The Speaker is in **Room A** reading the finished samples. Simultaneously, the CPU is locked in **Room B** working on the next batch.
 > > 2. **Phase 2:** The instant the Speaker finishes Room A, the "Moderator" (the Audio Driver) swaps the signs on the doors.
 > > 3. **The Swap:** Now the Speaker walks into **Room B** (which the CPU just finished). The CPU is kicked into **Room A** to start over.
 > ### Why this matters for IEMs
 > > If you are building an In-Ear Monitor driver, you are building the thing that sits in that "Room." If your code doesn't finish its **Surgical Quantization** in time, the user hears a "pop" that could literally damage their hearing because the DC offset jumps instantly.
 > > If the CPU is too slow in Room B, the Moderator still swaps the doors because the "Audio Clock" waits for no one. The Speaker walks into Room B, but the CPU hasn't finished "cleaning" it. The Speaker sees half-finished math, random garbage, or zeros. **That is the "Glitch."**

```
STEP 1: [Hardware filling A]    [CPU processing B]    [Hardware playing C]
STEP 2: [Hardware filling B]    [CPU processing C]    [Hardware playing A]
STEP 3: [Hardware filling C]    [CPU processing A]    [Hardware playing B]
```

```
AUDIO INTERFACE (Hardware)             YOUR PROGRAM (CPU)
   __________________________             _________________________
  |                          |           |                         |
  |  [BUFFER A] <--- (Mic In)|           |                         |
  |                          |           |                         |
  |  [BUFFER B]  (Wait...)   |           |                         |
  |__________________________|           |_________________________|
               |                                      ^
               |   EVENT: BUFFER A IS FULL!           |
               |------------------------------------->|
               |   (Interrupt / Callback Triggered)   |
               |                                      |
   ____________V_____________             ____________V____________
  |                          |           |                         |
  |  [BUFFER A] (Standby)    |           | [YOUR PROCESS LOOP]     |
  |                          |           | for(int i=0; i<128; i++){|
  |  [BUFFER B] <--- (Mic In)|           |    bufferA[i] = ...     |
  |                          |           | }                       |
  |__________________________|           |_________________________|
               ^                                      |
               |   EVENT: PROCESS FINISHED!           |
               |<-------------------------------------|
               |   (Handing back the pointer)         |
               |                                      |
   ____________V_____________             ____________V____________
  |                          |           |                         |
  |  [BUFFER A] -->(Speakers)|           |                         |
  |                          |           |                         |
  |  [BUFFER B]  (Wait...)   |           |                         |
  |__________________________|           |_________________________|
```

Our main goal while developing such audio processing software is to write efficient code which processes information in a very short span of time. This is the core goal for any program.

#definition_term_BufferUnderrun -> If the program ends up being slow and does not complete it's operations in the required time, the speaker tries to play information from the buffer but the buffer is empty (actually it has stale data from the previous buffer's tail - which causes a rhythmic "glitching" or "zipper noise" because the speaker is repeating the last 128 samples over and over) since your program is holding it. This causes a #term_BufferUnderrun.

> [!TIP] The Tradeoff
> * **Small Buffer (128 samples):** Low latency (good for live playing), but high CPU stress. Risk of clicks if the CPU "hiccups."
> * **Large Buffer (1024 samples):** High stability (good for mixing), but noticeable delay between pressing a key and hearing sound.

# [[01_basic_glitch_simulator.cpp|GLITCH SIMULATOR]]
I wanted to create a simulation of a "glitch" or a "pop" sound that we hear sometimes.

## The Physics
Basically, imagine a grid laid out. Let's say this is a 16-bit recording with a buffer size of 128. Now, when there is a drastic change in the amplitude, a few things happen.
1. Since audio levels vary from -1.0 to 1.0, it is very easy for the CPU to calculate these changes and alter the snapshot.
2. The problem comes with the speakers. A speaker operates by vibrating. This means it manipulates sound pressure. If for example, in the buffer array at 50th index the amplitude was 1.0 and at the 51st index is at -1.0, the speaker basically needs to move an infinite distance in an instant. The speaker tries its best to follow this voltage change. The speaker is already pushed out and vibrating at it's peak and is expected to suddenly pull the diaphragm back in. This causes a difference in pressure which is enormous. THIS is the "glitch" or the "pop" we hear. The instant attempt to pull back the diaphragm. Of course, when going from -1.0 to 1.0, the movement is reversed, yet the core logic is the same.

### Explaining the Simulation
When you run the simulation, you will observe vertical red lines. These lines are the representation of the "physically impossible" movement.

![[tyli/Audio DSP/01_Phase_1_Master_Bit_Crusher/01_Amplitude_Logic/01_Sample_Rate_and_Bit_Rate/01_image_1.png]]
	Notice how the speaker has to go from near zero to the highest level and then after 2 samples again drop down to near zero.
