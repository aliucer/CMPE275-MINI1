#pragma once
#include <cstddef>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

#ifdef _OPENMP
#include <omp.h>
#endif

#include "CSVParser.hpp"
#include "IDataStore.hpp"
#include "Record311.hpp"

namespace nyc311 {

class ParallelDataStore : public IDataStore {
public:
    void load(const std::string& path) override {
        CSVParser<Record311> parser(path);
        if (!parser.open()) throw std::runtime_error("Cannot open: " + path);
        ColumnMap cm = ColumnMap::fromHeader(parser.headerFields());
        Record311 rec;
        while (parser.readNext(rec, cm))
            records_.push_back(std::move(rec));
        parser.close();
    }

    void loadMultiple(const std::vector<std::string>& paths) override {
        int n = static_cast<int>(paths.size());
        std::vector<std::vector<Record311>> per_file(n);

        #pragma omp parallel for schedule(dynamic, 1)
        for (int i = 0; i < n; ++i) {
            CSVParser<Record311> parser(paths[i]);
            if (!parser.open()) continue;
            ColumnMap cm = ColumnMap::fromHeader(parser.headerFields());
            Record311 rec;
            while (parser.readNext(rec, cm))
                per_file[i].push_back(std::move(rec));
            parser.close();
        }

        size_t total = 0;
        for (auto& v : per_file) total += v.size();
        records_.reserve(records_.size() + total);
        for (auto& v : per_file)
            records_.insert(records_.end(),
                std::make_move_iterator(v.begin()),
                std::make_move_iterator(v.end()));
    }

    size_t size() const override { return records_.size(); }

    size_t memoryBytes() const override {
        return records_.capacity() * sizeof(Record311);
    }

    std::vector<size_t> filterByBorough(const std::string& b) const override {
        Borough target = boroughFromString(b);
        std::vector<size_t> result;
        #pragma omp parallel
        {
            std::vector<size_t> local;
            #pragma omp for nowait schedule(static)
            for (size_t i = 0; i < records_.size(); ++i)
                if (records_[i].borough == target) local.push_back(i);
            #pragma omp critical
            result.insert(result.end(), local.begin(), local.end());
        }
        return result;
    }

    std::vector<size_t> filterByComplaintType(const std::string& type) const override {
        std::vector<size_t> result;
        #pragma omp parallel
        {
            std::vector<size_t> local;
            #pragma omp for nowait schedule(static)
            for (size_t i = 0; i < records_.size(); ++i)
                if (records_[i].complaint_type == type) local.push_back(i);
            #pragma omp critical
            result.insert(result.end(), local.begin(), local.end());
        }
        return result;
    }

    std::vector<size_t> filterByGeoBox(double minLat, double maxLat,
                                        double minLon, double maxLon) const override {
        std::vector<size_t> result;
        #pragma omp parallel
        {
            std::vector<size_t> local;
            #pragma omp for nowait schedule(static)
            for (size_t i = 0; i < records_.size(); ++i) {
                double la = records_[i].latitude, lo = records_[i].longitude;
                if (la >= minLat && la <= maxLat && lo >= minLon && lo <= maxLon)
                    local.push_back(i);
            }
            #pragma omp critical
            result.insert(result.end(), local.begin(), local.end());
        }
        return result;
    }

    std::vector<size_t> filterByDateRange(uint32_t s, uint32_t e) const override {
        std::vector<size_t> result;
        #pragma omp parallel
        {
            std::vector<size_t> local;
            #pragma omp for nowait schedule(static)
            for (size_t i = 0; i < records_.size(); ++i) {
                uint32_t d = records_[i].created_ymd;
                if (d >= s && d <= e) local.push_back(i);
            }
            #pragma omp critical
            result.insert(result.end(), local.begin(), local.end());
        }
        return result;
    }

    std::tuple<double, double, size_t> computeCentroid() const override {
        double sum_lat = 0, sum_lon = 0;
        size_t valid = 0;
        #pragma omp parallel for reduction(+:sum_lat,sum_lon,valid) schedule(static)
        for (size_t i = 0; i < records_.size(); ++i) {
            if (records_[i].latitude != 0.0 && records_[i].longitude != 0.0) {
                sum_lat += records_[i].latitude;
                sum_lon += records_[i].longitude;
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
