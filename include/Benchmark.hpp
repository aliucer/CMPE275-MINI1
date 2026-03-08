#pragma once
#include <algorithm>
#include <chrono>
#include <cstddef>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>

namespace nyc311 {

class Timer {
public:
    void start() { t0_ = Clock::now(); }
    void stop()  { t1_ = Clock::now(); }
    double elapsedMs() const {
        return std::chrono::duration<double, std::milli>(t1_ - t0_).count();
    }
    double elapsedSec() const { return elapsedMs() / 1000.0; }
private:
    using Clock = std::chrono::high_resolution_clock;
    Clock::time_point t0_, t1_;
};

struct BenchmarkResult {
    std::string label;
    double mean_ms;
    int runs;
    size_t result_count;

    void print() const {
        std::cout << std::fixed << std::setprecision(1)
                  << "  " << label << ": " << mean_ms << " ms"
                  << "  (" << result_count << " hits)\n";
    }

    void writeCsv(std::ofstream& out) const {
        out << std::fixed << std::setprecision(3)
            << label << "," << runs << "," << mean_ms << ","
            << result_count << "\n";
    }
};

class Benchmark {
public:
    template<typename Fn>
    static BenchmarkResult run(const std::string& label, int iters, Fn&& fn) {
        std::vector<double> times;
        times.reserve(iters);
        size_t last_count = 0;
        Timer t;

        for (int i = 0; i < iters; ++i) {
            t.start();
            last_count = fn();
            t.stop();
            times.push_back(t.elapsedMs());
        }

        double mean = std::accumulate(times.begin(), times.end(), 0.0) / times.size();
        return {label, mean, iters, last_count};
    }

    static void writeCsvHeader(std::ofstream& out) {
        out << "label,runs,mean_ms,result_count\n";
    }
};

}
