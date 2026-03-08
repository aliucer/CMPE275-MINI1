# CMPE-273 Mini Project 1: Memory Overload

**Course:** CMPE-273, Spring 2026
**Team:** Anukrithi Myadala, Asim Mohammed, Ali Ucer
**Dataset:** NYC 311 Service Requests (2020–Present), ~18M records, ~12 GB CSV
**Source:** https://data.cityofnewyork.us/Social-Services/311-Service-Requests-from-2020-to-Present/erm2-nwe9

---

## 1. Introduction

This project investigates how memory layout affects query performance on large datasets. We use the NYC 311 Service Requests dataset and implement three phases:

- Phase 1: Serial Array-of-Structures (AoS) — our baseline
- Phase 2: OpenMP-parallelized AoS — adding threads to the same layout
- Phase 3: Structure-of-Arrays (SoA) with vectorization — redesigning the data layout

Our central question: does changing how data is arranged in memory matter more than adding CPU threads?

---

## 2. Background

**The Cache Line Problem.** Modern CPUs read memory in 64-byte chunks called cache lines. When a program needs just 1 byte, the CPU fetches the entire 64-byte line. If the surrounding bytes are useful (spatial locality), this is efficient. If they are irrelevant, the bandwidth is wasted.

**AoS vs SoA.** In AoS, each record is one contiguous block — a query filtering by `borough` must load the entire record (all fields). In SoA, each field is in its own array — a borough filter reads only the `boroughs[]` array, so every byte loaded is relevant.

**SIMD.** When data is contiguous and homogeneous (as in SoA), compilers can auto-vectorize loops using SIMD instructions. A single AVX2 instruction can compare 8 integers or 4 doubles simultaneously.

---

## 3. Dataset

The NYC 311 dataset has 44 columns per row, but we only parse 5 — the minimum needed for our queries:

- `unique_key` (column 0) → `uint64_t`, 8 bytes — record identity
- `created_date` (column 1) → `uint32_t` as YYYYMMDD, 4 bytes — date range filter
- `borough` (column 28) → `enum Borough : uint8_t`, 1 byte — borough filter
- `latitude` (column 41) → `double`, 8 bytes — geo-box filter, centroid
- `longitude` (column 42) → `double`, 8 bytes — geo-box filter, centroid

Total per record: 29 bytes. The original codebase parsed all 44 columns into a ~500-byte struct — a 17× difference.

**Date parsing.** NYC 311 dates come as `"03/09/2024 01:12:45 AM"` (MM/DD/YYYY). We convert to YYYYMMDD integers using character arithmetic — zero heap allocations:

```cpp
// "03/09/2024 01:12:45 AM" → 20240309
static uint32_t parseDate(const std::string& s) {
    if (s.size() < 10 || s[2] != '/')
        throw std::invalid_argument("bad date: " + s);
    uint32_t m = (s[0]-'0')*10 + (s[1]-'0');
    uint32_t d = (s[3]-'0')*10 + (s[4]-'0');
    uint32_t y = (s[6]-'0')*1000 + (s[7]-'0')*100 + (s[8]-'0')*10 + (s[9]-'0');
    return y * 10000 + m * 100 + d;
}
```

---

## 4. Architecture

All three phases implement the same `IDataStore` interface — 4 queries:

- `filterByBorough(string)` → indices of matching records
- `filterByGeoBox(minLat, maxLat, minLon, maxLon)` → indices
- `filterByDateRange(startYmd, endYmd)` → indices
- `computeCentroid()` → mean lat/lon across all valid records

A single `runAllBenchmarks(IDataStore& store)` function works with all three store types through polymorphism. This is the key use of the interface: one benchmark function, three different backends.

**Files:**

- `Record311.hpp` — struct (5 fields), date parser, borough enum
- `CSVParser.hpp` — template CSV reader
- `IDataStore.hpp` — abstract interface
- `DataStore.hpp` — Phase 1 serial AoS
- `ParallelDataStore.hpp` — Phase 2 OpenMP AoS
- `VectorStore.hpp` — Phase 3 SoA
- `Benchmark.hpp` — timer, result struct
- `BenchmarkRunner.hpp` — shared benchmark logic (polymorphic)
- `src/phase{1,2,3}/main.cpp` — entry points

Total: ~550 lines of C++.

---

## 5. Implementation

### Phase 1: Serial AoS (DataStore)

Records stored as `std::vector<Record311>`. Each query is a simple `for` loop:

```cpp
std::vector<size_t> filterByBorough(const std::string& b) const override {
    Borough target = boroughFromString(b);
    std::vector<size_t> result;
    for (size_t i = 0; i < records_.size(); ++i)
        if (records_[i].borough == target) result.push_back(i);
    return result;
}
```

A borough filter reads only 1 byte (`borough`) but loads the entire 29-byte struct per record. Cache utilization is about 3%.

### Phase 2: OpenMP AoS (ParallelDataStore)

Same data layout as Phase 1. Queries parallelized with thread-local vectors merged via `#pragma omp critical`:

```cpp
#pragma omp parallel
{
    std::vector<size_t> local;
    #pragma omp for nowait schedule(static)
    for (size_t i = 0; i < records_.size(); ++i)
        if (records_[i].borough == target) local.push_back(i);
    #pragma omp critical
    result.insert(result.end(), local.begin(), local.end());
}
```

`loadMultiple()` loads multiple CSV files in parallel using `#pragma omp parallel for`.

### Phase 3: SoA (VectorStore)

Each field stored in its own contiguous vector:

```cpp
std::vector<uint64_t> unique_keys_;
std::vector<uint32_t> created_ymds_;
std::vector<Borough>  boroughs_;
std::vector<double>   latitudes_;
std::vector<double>   longitudes_;
```

A borough filter reads only `boroughs_` — 1 byte per record, packed contiguously. Cache utilization is ~100%.

`loadMultiple()` creates per-thread `VectorStore` objects and merges column-by-column. There is no AoS intermediate — SoA-to-SoA merge avoids the 2× peak RAM problem.

---

## 6. Benchmark Design

We chose 4 queries that exercise different data types and access patterns:

- **Q1 (Borough filter):** Reads a 1-byte enum per record. Best case for SoA since minimal bytes per record are needed.
- **Q2 (Geo-box filter):** Reads two doubles (16 bytes) per record. Tests multi-column access.
- **Q3 (Date range filter):** Reads a single uint32 (4 bytes) per record. SIMD-friendly integer comparison.
- **Q4 (Centroid):** Floating-point reduction over lat/lon. Tests combined SIMD and OpenMP reduction.

Each query runs 12 iterations. We report mean latency in milliseconds. Results are written to CSV.

---

## 7. Results

### Test Environment

- OS: Ubuntu 24.04 (WSL2)
- Processor: TODO (`lscpu`)
- Memory: TODO (`free -h`)
- Compiler: GCC 13, `-O3 -march=native`
- OpenMP threads: 12

### Load Times

TODO: Fill after 18M test.

- Phase 1 (Serial AoS): TODO seconds, TODO MB
- Phase 2 (Parallel AoS, 12 threads): TODO seconds, TODO MB
- Phase 3 (Parallel SoA, 12 threads): TODO seconds, TODO MB

### Query Latency — 900K Records (~700 MB)

- Q1 Borough: Phase 1 = 4.5 ms, Phase 2 = 6.3 ms, Phase 3 = 3.4 ms (1.3× speedup)
- Q2 GeoBox: Phase 1 = 7.3 ms, Phase 2 = 4.4 ms, Phase 3 = 1.6 ms (4.5× speedup)
- Q3 Date: Phase 1 = 1.7 ms, Phase 2 = 4.6 ms, Phase 3 = 0.1 ms (17× speedup)
- Q4 Centroid: Phase 1 = 1.6 ms, Phase 2 = 6.3 ms, Phase 3 = 0.3 ms (5.3× speedup)

Memory: Phase 1/2 = 32 MB, Phase 3 = 29 MB.

### Query Latency — 18M Records (~12 GB)

TODO: Fill after full dataset test.

- Q1 Borough: Phase 1 = TODO, Phase 2 = TODO, Phase 3 = TODO
- Q2 GeoBox: Phase 1 = TODO, Phase 2 = TODO, Phase 3 = TODO
- Q3 Date: Phase 1 = TODO, Phase 2 = TODO, Phase 3 = TODO
- Q4 Centroid: Phase 1 = TODO, Phase 2 = TODO, Phase 3 = TODO

Memory: Phase 1/2 = TODO MB, Phase 3 = TODO MB.

### Thread Scaling (Phase 3, Q2 GeoBox)

TODO: Run with `OMP_NUM_THREADS=1,2,4,8,12` and record.

- 1 thread: TODO ms (baseline)
- 2 threads: TODO ms
- 4 threads: TODO ms
- 8 threads: TODO ms
- 12 threads: TODO ms

---

## 8. Analysis

TODO: Write after 18M results.

- Phase 2 vs Phase 1: does parallelization improve at 18M? At 900K it made things worse for 3/4 queries due to thread overhead.
- Phase 3 SoA cache efficiency: calculate how much each query's cache utilization improves.
- Memory-bound vs compute-bound: determine arithmetic intensity for each query.
- Memory footprint comparison across phases.

---

## 9. Approaches We Tried

**Date parsing with strptime().** We first tried `strptime()` but it is not available on all platforms (MSVC on Windows). We switched to character arithmetic — zero heap allocations, fully portable.

**Parsing all 44 columns.** The original codebase parsed every column in the CSV into a ~500-byte struct. We found that only 5 columns were actually queried. Removing the other 39 reduced struct size to 29 bytes and eliminated millions of unnecessary string allocations during loading.

**Parallel merge strategies.** When merging thread-local results in Phase 2, we tried three approaches. First, shared vector with `push_back` — immediate data race. Second, `mutex` per `push_back` — thread-safe but extremely slow due to millions of lock acquisitions. Third, thread-local vectors with a single critical merge at the end — this is what we use.

**VectorStore loading through AoS intermediate.** The original Phase 3 loaded into `vector<Record311>` first, then scattered fields into SoA vectors. Both copies existed simultaneously — 2× peak RAM. We rewrote it to create per-thread `VectorStore` objects and merge column-by-column, eliminating the intermediate.

---

## 10. Conclusions

TODO: Write after 18M results.

Expected topics:

1. Data layout (AoS vs SoA) impact on query latency
2. Parallelization effectiveness at different dataset sizes
3. Memory footprint comparison
4. Whether the workload is memory-bound or compute-bound

---

## Appendix A: Team Contributions

- Anukrithi Myadala: TODO
- Asim Mohammed: TODO
- Ali Ucer: TODO

## Appendix B: Build Instructions

```bash
sudo apt install build-essential cmake libomp-dev
cd CMPE-273-MINI1-ANU
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j

# Download dataset (not included)
curl -o data/311_data.csv \
  "https://data.cityofnewyork.us/api/views/erm2-nwe9/rows.csv?accessType=DOWNLOAD"

./build/src/phase1/phase1 data/311_data.csv
./build/src/phase2/phase2 data/311_data.csv
./build/src/phase3/phase3 data/311_data.csv
```

## Appendix C: Archive Contents

```
CMPE-273-MINI1-ANU/
├── CMakeLists.txt
├── include/
│   ├── IDataStore.hpp, Record311.hpp, CSVParser.hpp
│   ├── DataStore.hpp, ParallelDataStore.hpp, VectorStore.hpp
│   ├── Benchmark.hpp, BenchmarkRunner.hpp
├── src/
│   ├── phase1/main.cpp + CMakeLists.txt
│   ├── phase2/main.cpp + CMakeLists.txt
│   └── phase3/main.cpp + CMakeLists.txt
├── data/generate_sample.py
├── report/report_masters.md
└── mini1.md
```

---

*End of Report*
