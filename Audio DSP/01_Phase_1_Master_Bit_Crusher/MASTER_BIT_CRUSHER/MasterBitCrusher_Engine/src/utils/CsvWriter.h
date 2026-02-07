#pragma once
#include <string>
#include <vector>
#include <fstream>

class CsvWriter {
public:
    void open(const std::string& fileName);
    void writeHeader(const std::vector<std::string>& columns);
    void writeRow(const std::vector<double>& values);
    void close();
private:
    std::ofstream file_;
};