#pragma once
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace nyc311 {

template<typename RecordType>
class CSVParser {
public:
    explicit CSVParser(const std::string& filepath)
        : filepath_(filepath), lines_read_(0) {}

    bool open() {
        file_.open(filepath_);
        if (!file_.is_open()) return false;
        std::string header;
        if (!std::getline(file_, header)) return false;
        return true;
    }

    void close() {
        if (file_.is_open()) file_.close();
    }

    bool readNext(RecordType& record) {
        std::string line;
        while (std::getline(file_, line)) {
            ++lines_read_;
            if (line.empty()) continue;
            auto fields = splitRow(line);
            if (RecordType::fromFields(fields, record)) return true;
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
};

}
