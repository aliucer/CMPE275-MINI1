# Memory Overload — NYC 311 Service Requests

**CMPE 275 — Mini Project 1**  
**Team:** [Name 1], [Name 2], [Name 3]

## 1. Introduction

We used the NYC 311 Service Requests dataset from NYC OpenData. 311 is New York's non-emergency complaint system — residents report issues like noise, illegal parking, broken streetlights, etc. We downloaded data from 2020 through 2025 using the Socrata API, split into 72 monthly CSV files. Total: 20,403,336 records, roughly 13 GB on disk.

All benchmarks ran on Google Colab with an AMD EPYC 7B12 (8 cores) and 50 GB RAM. We used g++ with `-O2` optimization.

## 2. Design

### Data Representation

The assignment says to pay attention to primitive types, so we converted fields at parse time instead of storing raw strings:

- **Borough:** 5 possible values → `enum Borough : uint8_t` (1 byte instead of ~10 byte string)
- **Date:** `"2020-01-15T00:00:00.000"` → `uint32_t` as `20200115` (4 bytes instead of 24 byte string)
- **Lat/Lon:** parsed to `double`
- **Complaint type:** kept as `std::string` intentionally — we wanted to compare string vs primitive query performance

Each record is about 60 bytes. A naive all-string version would be 500+.

### Class Structure

```
IDataStore (abstract interface)
├── DataStore           — Phase 1: serial, vector<Record311>
├── ParallelDataStore   — Phase 2: OpenMP, vector<Record311>
└── VectorStore         — Phase 3: SoA, one vector per column
```

All three implement the same interface, so the benchmark runner works with any phase without changes. `CSVParser<T>` is a template that reads the header row and builds a `ColumnMap` to find columns by name — this way it handles any column ordering.

### Queries

We picked 5 queries that stress different memory access patterns:

- **Q1_borough** — scans 1-byte enum array
- **Q2_string** — scans heap-allocated strings (added to show string cost)
- **Q3_geobox** — checks two doubles (lat + lon) per record
- **Q4_date** — checks one uint32_t per record
- **Q5_centroid** — reduction: sums all lat/lon, no output vector

## 3. Phase 1 — Serial (AoS)

`std::vector<Record311>` — standard Array of Structs. Single-threaded loading and queries.

**Load:** 109.1 s | **Memory:** 2,304 MB | **Records:** 20,403,336

| Query | Mean (ms) | Hits |
|-------|-----------|------|
| Q1_borough | 150.2 | 6,094,561 |
| Q2_string | 181.1 | 2,346,948 |
| Q3_geobox | 223.9 | 6,447,701 |
| Q4_date | 106.8 | 3,169,960 |
| Q5_centroid | 94.8 | 20,037,886 |

All queries finish under 225 ms on 20M records. This is already fast compared to string-heavy implementations because we use primitive types. Q3_geobox is slowest (two double comparisons per record), Q4_date is fastest filter (single integer comparison).

## 4. Phase 2 — OpenMP (AoS)

Same data layout as Phase 1, but with OpenMP:

- **File loading:** `#pragma omp parallel for schedule(dynamic, 1)` — one thread per CSV file
- **Filters:** thread-local vectors merged via `#pragma omp critical`
- **Centroid:** `#pragma omp reduction(+:sum_lat, sum_lon)` — no lock needed

**Load:** 24.3 s | **Memory:** 1,400 MB | **Threads:** 8

| Query | Mean (ms) | vs Phase 1 |
|-------|-----------|------------|
| Q1_borough | 77.6 | 1.9× |
| Q2_string | 44.6 | 4.1× |
| Q3_geobox | 77.3 | 2.9× |
| Q4_date | 33.4 | 3.2× |
| Q5_centroid | 27.7 | 3.4× |

Loading improved 4.5× (109s → 24s). Query speedups range 1.9×–4.1× on 8 threads, well short of 8×. The queries are memory-bandwidth-bound — adding threads doesn't give more memory bandwidth. The `omp critical` merge also adds overhead when multiple threads try to append results simultaneously.

## 5. Phase 3 — SoA (Structure of Arrays)

Instead of one vector of structs, each field gets its own vector:

```cpp
vector<Borough>     boroughs_;        // 20M × 1 byte
vector<uint32_t>    created_ymds_;    // 20M × 4 bytes
vector<string>      complaint_types_; // 20M × ~40 bytes (heap)
vector<double>      latitudes_;       // 20M × 8 bytes
vector<double>      longitudes_;      // 20M × 8 bytes
```

Each query only reads the arrays it needs.

**Load:** 27.0 s | **Memory:** 1,993 MB

| Query | Mean (ms) | vs Phase 1 | vs Phase 2 |
|-------|-----------|------------|------------|
| Q1_borough | 68.5 | 2.2× | 1.1× |
| Q2_string | 38.6 | 4.7× | 1.2× |
| Q3_geobox | 81.7 | 2.7× | 0.95× ← worse |
| Q4_date | 37.3 | 2.9× | 0.89× ← worse |
| Q5_centroid | 8.6 | **11.0×** | **3.2×** |

### Where SoA helped

**Q5_centroid** went from 94.8 ms to 8.6 ms (11× faster). It's a pure reduction — no output vector, no push_back, just summing two contiguous double arrays. The compiler can auto-vectorize this with SIMD, and `omp reduction` avoids any locking.

### Where SoA didn't help

**Q3_geobox** got slightly worse. It needs both lat and lon per record. In AoS they sit next to each other in the same struct (one cache line). In SoA they're in separate arrays — the CPU has to read from two different memory locations per record.

**Q1, Q2, Q4** showed small improvements in scan speed, but the bottleneck is the output: these queries return millions of indices (Q1 returns 6M hits), and `push_back` cost is the same regardless of memory layout.

## 6. String Experiment

We ran the same benchmarks with and without the `complaint_type` string field:

- **Without strings:** Phase 1 memory = 1,024 MB
- **With one string:** Phase 1 memory = 2,304 MB

One `std::string` field doubled memory. Each of the 20M records carries a heap allocation for the complaint text.

In Phase 3, Q2_string (simple equality check) took 38.6 ms while Q5_centroid (summing 20M coordinate pairs) took 8.6 ms. A string comparison is 4.5× slower than actual math because each string lives in a separate heap location — the CPU can't prefetch them efficiently.

## 7. Failed Attempts & Lessons Learned

### 7.1 Local Machine — Swap Thrashing
First attempt was on a laptop with 16 GB RAM. Loading 13 GB of CSV consumed all physical memory and swapped to disk. Load time went from ~100s to 400+ seconds. We moved to Colab (50 GB) to get real memory benchmarks.

### 7.2 Socrata API & CSV Parsing
Socrata API limits rows per request — our first download only got 1,000 rows. We split downloads by month (72 files) and parallelized with `xargs -P 12`.

The first parser used hardcoded column indices. Socrata exports have different column order and lowercase headers. The parser produced 0 records without any errors. We fixed this by reading the header row and looking up columns by name (`ColumnMap`).

### 7.3 `omp critical` vs `omp reduction`
First centroid implementation used `omp critical` — all 8 threads competed for one lock on every iteration. Scaling was terrible. Switching to `omp reduction(+:)` let each thread accumulate locally and merge once at the end. This alone took centroid from ~80 ms to ~28 ms in Phase 2.

### 7.4 SoA — Not Always Faster
We expected SoA to improve everything. It didn't. GeoBox got slightly worse because lat and lon got separated into different arrays. Filter queries with millions of hits didn't improve much because the bottleneck shifted from scanning to output construction (push_back). SoA is great for reductions and single-column scans, but not for multi-column queries.

### 7.5 `std::move` Semantics
Early code used `push_back(rec)` which copies the string field for every record — 20M heap allocations. Using `push_back(std::move(rec))` transfers ownership without copying.

## 8. Conclusions

1. **Primitive types matter most.** Converting strings to enums and integers at parse time cut memory from ~15 GB to ~1 GB and made serial queries 10× faster.

2. **OpenMP helps I/O more than queries.** File loading got 4.5× faster. Queries only 2–4× because they're memory-bound.

3. **SoA is great for reductions.** Centroid got 11× faster. But for filter queries that output millions of results, the push_back cost dominates and SoA barely helps.

4. **SoA can be worse.** GeoBox queries need two related columns that AoS keeps together. Separating them into different arrays broke co-locality.

5. **One string doubles memory.** Adding a single `std::string` field to 20M records doubled the footprint and made simple equality checks 4.5× slower than math operations on contiguous arrays.

## 9. Individual Contributions

| Member | Contributions |
|--------|--------------|
| [Name 1] | TODO |
| [Name 2] | TODO |
| [Name 3] | TODO |

## 10. References

- NYC OpenData 311 Service Requests: https://data.cityofnewyork.us/resource/erm2-nwe9
- OpenMP Specification: https://www.openmp.org/specifications/
- Mike Acton, "Data-Oriented Design and C++", CppCon 2014
- Intel 64 and IA-32 Architectures Optimization Reference Manual
- cppreference.com — std::string, Small String Optimization

## Appendix: Hardware

| | |
|---|---|
| CPU | AMD EPYC 7B12 |
| Cores | 8 |
| RAM | 50 GB |
| OS | Linux 6.6.113+ x86_64 |
| Compiler | g++ -O2 |
| Platform | Google Colab |
