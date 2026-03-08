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

class VectorStore : public IDataStore {
public:
    void load(const std::string& path) override {
        CSVParser<Record311> parser(path);
        if (!parser.open()) throw std::runtime_error("Cannot open: " + path);
        Record311 rec;
        while (parser.readNext(rec))
            appendRecord(rec);
        parser.close();
    }

    void loadMultiple(const std::vector<std::string>& paths) override {
        int n = static_cast<int>(paths.size());
        std::vector<VectorStore> per_file(n);

        #pragma omp parallel for schedule(dynamic, 1)
        for (int i = 0; i < n; ++i)
            per_file[i].load(paths[i]);

        for (auto& vs : per_file) merge(vs);
    }

    size_t size() const override { return boroughs_.size(); }

    size_t memoryBytes() const override {
        size_t n = boroughs_.capacity();
        return n * (sizeof(uint64_t) + sizeof(uint32_t) + sizeof(Borough)
                  + sizeof(double) * 2);
    }

    std::vector<size_t> filterByBorough(const std::string& b) const override {
        Borough target = boroughFromString(b);
        size_t n = boroughs_.size();
        std::vector<size_t> result;
        #pragma omp parallel
        {
            std::vector<size_t> local;
            #pragma omp for nowait schedule(static)
            for (size_t i = 0; i < n; ++i)
                if (boroughs_[i] == target) local.push_back(i);
            #pragma omp critical
            result.insert(result.end(), local.begin(), local.end());
        }
        return result;
    }

    std::vector<size_t> filterByGeoBox(double minLat, double maxLat,
                                        double minLon, double maxLon) const override {
        size_t n = latitudes_.size();
        std::vector<size_t> result;
        #pragma omp parallel
        {
            std::vector<size_t> local;
            #pragma omp for nowait schedule(static)
            for (size_t i = 0; i < n; ++i) {
                double la = latitudes_[i], lo = longitudes_[i];
                if (la >= minLat && la <= maxLat && lo >= minLon && lo <= maxLon)
                    local.push_back(i);
            }
            #pragma omp critical
            result.insert(result.end(), local.begin(), local.end());
        }
        return result;
    }

    std::vector<size_t> filterByDateRange(uint32_t s, uint32_t e) const override {
        size_t n = created_ymds_.size();
        std::vector<size_t> result;
        #pragma omp parallel
        {
            std::vector<size_t> local;
            #pragma omp for nowait schedule(static)
            for (size_t i = 0; i < n; ++i) {
                uint32_t d = created_ymds_[i];
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
        size_t n = latitudes_.size();
        #pragma omp parallel for reduction(+:sum_lat,sum_lon,valid) schedule(static)
        for (size_t i = 0; i < n; ++i) {
            if (latitudes_[i] != 0.0 && longitudes_[i] != 0.0) {
                sum_lat += latitudes_[i];
                sum_lon += longitudes_[i];
                ++valid;
            }
        }
        if (valid == 0) return {0, 0, 0};
        return {sum_lat / valid, sum_lon / valid, valid};
    }

private:
    std::vector<uint64_t> unique_keys_;
    std::vector<uint32_t> created_ymds_;
    std::vector<Borough>  boroughs_;
    std::vector<double>   latitudes_;
    std::vector<double>   longitudes_;

    void appendRecord(const Record311& r) {
        unique_keys_.push_back(r.unique_key);
        created_ymds_.push_back(r.created_ymd);
        boroughs_.push_back(r.borough);
        latitudes_.push_back(r.latitude);
        longitudes_.push_back(r.longitude);
    }

    void merge(VectorStore& other) {
        auto mv = [](auto& dst, auto& src) {
            dst.insert(dst.end(),
                std::make_move_iterator(src.begin()),
                std::make_move_iterator(src.end()));
        };
        mv(unique_keys_, other.unique_keys_);
        mv(created_ymds_, other.created_ymds_);
        mv(boroughs_, other.boroughs_);
        mv(latitudes_, other.latitudes_);
        mv(longitudes_, other.longitudes_);
    }
};

}
