/*
* 1. Monitor input signal.
* 2. Use an epsilon (1e - 7) instead of hard 0.0f to detect silence.
* 3. If silence persists for 512 samples, the dither must drop to 0.0f.
* 4. The dither must "wake up" the very instant the signal crosses the threshold again.
*/