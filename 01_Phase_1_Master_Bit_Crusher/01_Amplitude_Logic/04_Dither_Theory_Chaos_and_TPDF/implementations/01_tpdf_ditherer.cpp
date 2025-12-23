#include <iostream>
#include <cmath>

/*
* Implementing a TPDF dither using the sum of 2 independent RPDF sources
*	1. The dither signal must be exactly [-1.0, 1.0] LSB peak-to-peak. No dividing the sum by 2.
*   2. Use a fast Linear Conruential Generator (LCG) or Xorshift.
*/
class TPDFDitherer {
public:
	float process_sample(float input)

private:
	int bit_depth = 16;
	int silence_counter = 0;

};