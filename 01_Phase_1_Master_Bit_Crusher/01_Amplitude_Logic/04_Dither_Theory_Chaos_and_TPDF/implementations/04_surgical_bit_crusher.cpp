/*
* Accept bit_depth and rate_divider as an input.
* Ensure all calculations are real-time safe (no new, no malloc, no std::pow) inside the sample loop.
* Pre-calculate the scales
* Use the roundf() function to pull the "fuzz" to the nearest bit-depth grid line.
*/