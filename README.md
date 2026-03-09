# CMPE-275 Mini Project 1

NYC 311 Service Requests — in-memory analytics comparing AoS vs SoA data layouts.

**Dataset:** [NYC 311 (2020–Present)](https://data.cityofnewyork.us/Social-Services/311-Service-Requests-from-2020-to-Present/erm2-nwe9) — ~18M records, ~12 GB CSV.

## Build

```bash
sudo apt install build-essential cmake libomp-dev   # if needed
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

## Get the Data

Run the included downloader script. This downloads the massive 13GB dataset month-by-month in parallel to prevent network timeouts:
```bash
chmod +x download_dataset.sh
./download_dataset.sh
```

Or generate a small test file:
```bash
python3 data/generate_sample.py 100000 data/test_sample.csv
```

## Run Benchmarks

Run the comprehensive benchmark script. This compiles the code in Release mode and benchmarks all 3 phases against the ~18M records, saving results to `benchmark_results.txt`:
```bash
chmod +x run_benchmarks.sh
./run_benchmarks.sh
```

Or run individually:
```bash
./build/src/phase1/phase1 data/nyc_311/311_*.csv          # serial AoS
./build/src/phase2/phase2 data/nyc_311/311_*.csv          # OpenMP AoS
./build/src/phase3/phase3 data/nyc_311/311_*.csv          # SoA vectorized
```

Each phase writes a `phaseN_results.csv` with benchmark timings.

## Team

Anukrithi Myadala, Asim Mohammed, Ali Ucer
