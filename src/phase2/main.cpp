#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <vector>

#ifdef _OPENMP
#include <omp.h>
#endif

#include "Benchmark.hpp"
#include "BenchmarkRunner.hpp"
#include "ParallelDataStore.hpp"

using namespace nyc311;

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: phase2 <csv> [csv ...]\n";
        return EXIT_FAILURE;
    }

    std::vector<std::string> files;
    for (int i = 1; i < argc; ++i) files.emplace_back(argv[i]);

    std::cout << "Phase 2: OpenMP AoS\n";
    #ifdef _OPENMP
    std::cout << "Threads: " << omp_get_max_threads() << "\n";
    #endif

    Timer t;
    t.start();
    ParallelDataStore store;
    if (files.size() == 1) store.load(files[0]);
    else store.loadMultiple(files);
    t.stop();

    std::cout << std::fixed << std::setprecision(1)
              << "Load: " << t.elapsedSec() << "s\n";

    runAllBenchmarks(store, "phase2_results.csv");
    return EXIT_SUCCESS;
}
