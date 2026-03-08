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

```bash
curl -o data/311_data.csv \
  "https://data.cityofnewyork.us/api/views/erm2-nwe9/rows.csv?accessType=DOWNLOAD"
```

Or generate a small test file:
```bash
python3 data/generate_sample.py 100000 /tmp/test.csv
```

## Run

```bash
./build/src/phase1/phase1 data/311_data.csv          # serial AoS
./build/src/phase2/phase2 data/311_data.csv           # OpenMP AoS
./build/src/phase3/phase3 data/311_data.csv           # SoA vectorized
```

Each phase writes a `phaseN_results.csv` with benchmark timings.

## Team

Anukrithi Myadala, Asim Mohammed, Ali Ucer
