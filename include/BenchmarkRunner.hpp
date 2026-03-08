#pragma once
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <tuple>
#include <vector>

#include "Benchmark.hpp"
#include "IDataStore.hpp"

namespace nyc311 {

inline void runAllBenchmarks(IDataStore& store, const std::string& csvOut) {
    std::cout << "Records: " << store.size()
              << " (" << store.memoryBytes() / (1024 * 1024) << " MB)\n\n";

    const int ITERS = 12;
    std::vector<BenchmarkResult> results;

    results.push_back(Benchmark::run("Q1_borough", ITERS, [&]() {
        return store.filterByBorough("BROOKLYN").size();
    }));
    results.push_back(Benchmark::run("Q2_geobox", ITERS, [&]() {
        return store.filterByGeoBox(40.700, 40.882, -74.020, -73.907).size();
    }));
    results.push_back(Benchmark::run("Q3_date", ITERS, [&]() {
        return store.filterByDateRange(20220101, 20221231).size();
    }));
    results.push_back(Benchmark::run("Q4_centroid", ITERS, [&]() {
        auto [la, lo, v] = store.computeCentroid();
        return v;
    }));

    std::cout << "Benchmarks (" << ITERS << " runs, mean):\n";
    for (auto& r : results) r.print();

    std::ofstream csv(csvOut);
    if (csv.is_open()) {
        Benchmark::writeCsvHeader(csv);
        for (auto& r : results) r.writeCsv(csv);
        std::cout << "\nResults: " << csvOut << "\n";
    }
}

}
