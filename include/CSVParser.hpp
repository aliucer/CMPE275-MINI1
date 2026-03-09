#pragma once
#include <fstream>
#include <string>
#include <vector>

#include "Record311.hpp"

namespace nyc311 {

template<typename RecordType>
class CSVParser {
public:
    explicit CSVParser(const std::string& filepath)
        : filepath_(filepath), lines_read_(0) {}

    bool open() {
        file_.open(filepath_);
        if (!file_.is_open()) return false;
        std::string header_line;
        if (!std::getline(file_, header_line)) return false;
        header_fields_ = splitRow(header_line);
        return true;
    }

    void close() { if (file_.is_open()) file_.close(); }

    const std::vector<std::string>& headerFields() const { return header_fields_; }

    bool readNext(RecordType& record, const ColumnMap& cm) {
        std::string line;
        while (std::getline(file_, line)) {
            ++lines_read_;
            if (line.empty()) continue;
            auto fields = splitRow(line);
            if (RecordType::fromFields(fields, cm, record)) return true;
        }
        return false;
    }

    size_t linesRead() const { return lines_read_; }

    static std::vector<std::string> splitRow(const std::string& line) {
        std::vector<std::string> fields;
        std::string field;
        bool in_quotes = false;

        for (size_t i = 0; i < line.size(); ++i) {
            char c = line[i];
            if (c == '"') {
                if (in_quotes && i + 1 < line.size() && line[i + 1] == '"') {
                    field += '"';
                    ++i;
                } else {
                    in_quotes = !in_quotes;
                }
            } else if (c == ',' && !in_quotes) {
                fields.push_back(std::move(field));
                field.clear();
            } else {
                field += c;
            }
        }
        fields.push_back(std::move(field));
        return fields;
    }

private:
    std::string filepath_;
    std::ifstream file_;
    size_t lines_read_;
    std::vector<std::string> header_fields_;
};

}
