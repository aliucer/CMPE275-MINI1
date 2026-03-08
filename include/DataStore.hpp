#pragma once
#include <cstddef>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

#include "CSVParser.hpp"
#include "IDataStore.hpp"
#include "Record311.hpp"

namespace nyc311 {

class DataStore : public IDataStore {
public:
    void load(const std::string& path) override {
        CSVParser<Record311> parser(path);
        if (!parser.open()) throw std::runtime_error("Cannot open: " + path);
        Record311 rec;
        while (parser.readNext(rec))
            records_.push_back(std::move(rec));
        parser.close();
    }

    void loadMultiple(const std::vector<std::string>& paths) override {
        for (auto& p : paths) load(p);
    }

    size_t size() const override { return records_.size(); }

    size_t memoryBytes() const override {
        return records_.capacity() * sizeof(Record311);
    }

    std::vector<size_t> filterByBorough(const std::string& b) const override {
        Borough target = boroughFromString(b);
        std::vector<size_t> result;
        for (size_t i = 0; i < records_.size(); ++i)
            if (records_[i].borough == target) result.push_back(i);
        return result;
    }

    std::vector<size_t> filterByGeoBox(double minLat, double maxLat,
                                        double minLon, double maxLon) const override {
        std::vector<size_t> result;
        for (size_t i = 0; i < records_.size(); ++i) {
            double la = records_[i].latitude, lo = records_[i].longitude;
            if (la >= minLat && la <= maxLat && lo >= minLon && lo <= maxLon)
                result.push_back(i);
        }
        return result;
    }

    std::vector<size_t> filterByDateRange(uint32_t s, uint32_t e) const override {
        std::vector<size_t> result;
        for (size_t i = 0; i < records_.size(); ++i) {
            uint32_t d = records_[i].created_ymd;
            if (d >= s && d <= e) result.push_back(i);
        }
        return result;
    }

    std::tuple<double, double, size_t> computeCentroid() const override {
        double sum_lat = 0, sum_lon = 0;
        size_t valid = 0;
        for (auto& r : records_) {
            if (r.latitude != 0.0 && r.longitude != 0.0) {
                sum_lat += r.latitude;
                sum_lon += r.longitude;
                ++valid;
            }
        }
        if (valid == 0) return {0, 0, 0};
        return {sum_lat / valid, sum_lon / valid, valid};
    }

private:
    std::vector<Record311> records_;
};

}
