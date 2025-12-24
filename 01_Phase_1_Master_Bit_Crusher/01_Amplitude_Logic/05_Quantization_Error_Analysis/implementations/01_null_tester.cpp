/*
 * NullTester.h
 * A diagnostic tool to isolate Quantization Error.
 * * Logic:
 * 1. Receive original Clean Input.
 * 2. Receive Processed (Crushed) Output.
 * 3. Calculate Difference (Residual).
 * 4. Apply "Microscope" gain (boost the error so we can hear/see it).
 */