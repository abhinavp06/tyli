/*
* In 02_Basic_Linear_Quantization, we created a 02_blocky_bit_rate_simulator.
* In this simulator, we will reimplement the sample and hold and the jitter logic;
*/

/*
* Implement a rate_divider using a phase accumulator.
* Ensure that the Sample-and-Hold happens before the Dither and Quantization.
* Store the sample as a member variable so the Dither logic is always acting on the most recently captured point in time.
*/