#pragma once
#include <vector>
#include <string>
#include <iostream>

class LinearityScope {
public:
    // Captures the Input vs Output relationship
    void feed(double input, double output);

    // Generates a CSV file to visualize the "Staircase"
    void saveCSV(const std::string& filename);

private:
    struct DataPoint {
        double input;
        double output;
    };
    std::vector<DataPoint> buffer_;
};