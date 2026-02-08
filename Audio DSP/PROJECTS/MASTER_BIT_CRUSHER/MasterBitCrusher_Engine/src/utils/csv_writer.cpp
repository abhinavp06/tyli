#include "csv_writer.h"
#include <iostream>

void CsvWriter::open(const std::string& fileName) {
	file_.open(fileName);
	if (!file_.is_open()) {
		std::cerr << "[Error] could not open CSV file: " << fileName << std::endl;
	}
}

void CsvWriter::writeHeader(const std::vector<std::string>& columns) {
	if (!file_.is_open()) return;

	for (size_t i = 0; i < columns.size(); ++i) {
		file_ << columns[i];

		if (i < columns.size() - 1) {
			file_ << ",";
		}
	}

	file_ << "\n";
}

void CsvWriter::writeRow(const std::vector<double>& values) {
	if (!file_.is_open()) return;

	for (size_t i=0; i<values.size(); ++i) {
		file_ << values[i];

		if (i < values.size() - 1) {
			file_ << ",";
		}
	}

	file_ << "\n";
}

void CsvWriter::close() {
	if (file_.is_open()) {
		file_.close();
	}
}