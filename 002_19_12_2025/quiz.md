### Part 1: The "Lego" Mechanics

**1. What is the fundamental difference between your first `glitch_simulator` and the `apply_bit_crush` engine?**

- A) The glitch simulator rounds values, the crusher moves them randomly.
- B) The glitch simulator moves samples to random locations; the crusher teleports them to the nearest "legal" grid line.
- C) There is no difference; they are both amplitude distorters.
- D) The glitch simulator changes the sample rate.

**2. If you are building a 4-bit Bit Crusher, how many "Lego Studs" (total levels) do you have available?**

- A) 4
- B) 8
- C) 16
- D) 32

**3. Why do we multiply the audio signal by $2^{N-1}$ before rounding?**

- A) To increase the volume of the audio.
- B) To project the signal into the "Integer World" where the rounding thresholds align with our LSB.
- C) To prevent clipping at -1.0.
- D) To convert the signal from Mono to Stereo.

**4. In a 3-bit system, why does the positive range only go to 3 while the negative goes to -4?**

- A) Because positive numbers are naturally smaller.
- B) It is an error in the `std::round` function.
- C) The "Zero Problem": One slot must be reserved for 0.0 (silence), stealing from the positive side.
- D) To account for the "Mud" bias.

---

### Part 2: The Physics of Failure

**5. What is the "Error Signal" (The Mud) in mathematical terms?**

- A) $Original + Processed$
- B) $Original \times BitDepth$
- C) $Processed - Original$
- D) $std::round(Original)$

**6. Why is the Mud described as "Correlated" or "Glued" to the music?**

- A) Because it is a random hiss that follows the volume.
- B) Because the rounding error repeats in a predictable pattern synchronized with the original wave.
- C) Because the bit depth is too high.
- D) Because the computer runs out of memory.

**7. You play a very quiet sine wave through a 3-bit crusher and hear a harsh "Buzz." What is happening?**

- A) The Gate has killed the signal.
- B) Limit Cycle Oscillation: the sine wave has been forced into a square wave.
- C) The Sample Rate is too low.
- D) The Transfer Function has become a diagonal line.

**8. What happens if your input signal amplitude is consistently below 0.5 LSB?**

- A) The signal becomes much louder.
- B) The signal becomes white noise.
- C) The Gate Effect: `std::round` pulls everything to 0.0, resulting in total silence.
- D) The signal becomes a Mid-Rise staircase.

---

### Part 3: Visualizer Logic

**9. In your "Only Mud" visualizer, what does a spike represent?**

- A) The volume of the music.
- B) The size of the gap between the "clay ball" (original) and the "Lego brick" (quantized).
- C) The Sample Rate.
- D) The frequency of the sine wave.

**10. Why do the red spikes in the "Only Mud" visualizer form a pattern rather than looking like random dust?**

- A) Because the rounding math is predictable and depends on the input shape.
- B) Because the `TOTAL_ROWS` constant is too small.
- C) Because of a bug in the coordinate mapper.
- D) Because the bit depth is set to 16.

**11. What is the maximum possible height of a "Mud spike" in terms of LSB?**

- A) 1.0 LSB
- B) 2.0 LSB
- C) 0.5 LSB
- D) $2^{N-1}$ LSB

---

### Part 4: Noise & Randomness

**12. What is the primary difference between Harmonic Distortion and White Noise?**

- A) Distortion is quiet; White Noise is loud.
- B) Distortion is periodic/predictable; White Noise is random/uncorrelated.
- C) Distortion happens in the terminal; White Noise happens in the speakers.
- D) There is no difference.

**13. To turn "Mud" into "Hiss" (White Noise), what is the missing ingredient?**

- A) A higher sample rate.
- B) More bits.
- C) Randomness.
- D) A Mid-Tread architecture.

**14. If a signal is a perfect horizontal line sitting exactly on a "stud" (grid line), what is the Error Signal?**

- A) 0.5 LSB
- B) 0.0
- C) 1.0
- D) Infinite

---

### Part 5: Advanced Architecture Concepts

**15. What is a "Transfer Function" in the context of your code?**

- A) A function that moves files between Obsidian and C++.
- B) The graph of Input vs. Output (The Staircase Map).
- C) The speed at which the visualizer renders.
- D) The math behind `std::pow`.

**16. What is the "Decision Boundary"?**

- A) The maximum volume of the system.
- B) The "line in the sand" (usually 0.5) where the math decides to jump to the next level.
- C) The width of the terminal window.
- D) The difference between 16-bit and 24-bit.

**17. In the "Lego Baseplate" analogy, how does Mid-Tread architecture handle the "Zero" position?**

- A) It puts a crack between two bricks at zero.
- B) It puts a flat brick centered at zero.
- C) It ignores the zero position.
- D) It makes the zero position random.

**18. Why does increasing Bit Depth from 3 to 8 make the "Mud" spikes smaller?**

- A) Because the Lego bricks (LSBs) become much smaller, reducing the maximum possible gap.
- B) Because the code runs faster.
- C) Because the sine wave becomes quieter.
- D) Because 8 is a lucky number in DSP.

**19. Which term describes the smallest possible "step" in a digital audio system?**

- A) MSB
- B) Sample
- C) LSB (Least Significant Bit)
- D) Delta

**20. What is the final step of the "Integer Bridge" bit crushing algorithm?**

- A) Multiplying by $2^{N-1}$
- B) Rounding
- C) Dividing back by $2^{N-1}$ to return to the Normalized Float domain.
- D) Adding White Noise.

---

**21. THE BORDERLINE CHALLENGE: In a 3-bit system ($LSB = 0.25$), an input of 0.1250001 results in an output of 0.25. If the input drops to 0.1249999, the output becomes 0.0. Why?**
- A) Because 0.125 is the "Decision Boundary" (Half-LSB) where `std::round` snaps to a different integer.
- B) Because the computer cannot process 7 decimal places.
- C) Because the negative range is larger than the positive range.
- D) Because the signal became non-periodic.

---
### 🗝️ Answer Key

<details>

<summary>Click to reveal answers</summary>

1. B | 2. C | 3. B | 4. C | 5. C | 6. B | 7. B | 8. C | 9. B | 10. A | 11. C | 12. B | 13. C | 14. B | 15. B | 16. B | 17. B | 18. A | 19. C | 20. C | 21. A    
</details>

Detailed Rationale for Question 21:
		In this 3-bit system, we scale the input by 4 ($2^{3-1}$).
		- For **0.1250001**: $0.1250001 \times 4 = 0.5000004$. `std::round` sees a value $> 0.5$ and snaps it to **1.0**. $1.0 / 4 = 0.25$.
		- For 0.1249999: $0.1249999 \times 4 = 0.4999996$. std::round sees a value $< 0.5$ and snaps it down to 0.0. $0.0 / 4 = 0.0$.
	    This prove that 0.125 is the exact "Half-LSB" threshold. This range between $-0.125$ and $+0.125$ is the Dead Zone of the Mid-Tread architecture, causing the "Gate Effect."
