#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <vector>

#include "Benchmark.hpp"
#include "BenchmarkRunner.hpp"
#include "DataStore.hpp"

using namespace nyc311;

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: phase1 <csv> [csv ...]\n";
        return EXIT_FAILURE;
    }

    std::vector<std::string> files;
    for (int i = 1; i < argc; ++i) files.emplace_back(argv[i]);

    std::cout << "Phase 1: Serial AoS\n";

    Timer t;
    t.start();
    DataStore store;
    store.loadMultiple(files); 
    t.stop();

    std::cout << std::fixed << std::setprecision(1)
              << "Load: " << t.elapsedSec() << "s\n";

    runAllBenchmarks(store, "phase1_results.csv");
    return EXIT_SUCCESS;
}
