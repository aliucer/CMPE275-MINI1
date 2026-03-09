#!/bin/bash
set -e

cd "$(dirname "$0")"

OUT="benchmark_results.txt"

echo "=============================================" | tee "$OUT"
echo "  NYC 311 Full Benchmark — $(date)"            | tee -a "$OUT"
echo "=============================================" | tee -a "$OUT"
echo ""                                               | tee -a "$OUT"

# ---- Hardware info ----
echo "--- Hardware Info ---"                          | tee -a "$OUT"
echo "CPU: $(lscpu | grep 'Model name' | sed 's/.*: *//')" | tee -a "$OUT"
echo "Cores: $(nproc)"                                | tee -a "$OUT"
echo "RAM: $(free -h | awk '/Mem:/{print $2}')"       | tee -a "$OUT"
echo "OS: $(uname -srm)"                              | tee -a "$OUT"
echo ""                                               | tee -a "$OUT"

# ---- Build (Release) ----
echo ">>> Building in Release mode..."                | tee -a "$OUT"
rm -rf build
cmake -B build -DCMAKE_BUILD_TYPE=Release 2>&1        | tail -3 | tee -a "$OUT"
cmake --build build -j 2>&1                            | tail -3 | tee -a "$OUT"
echo ">>> Build complete."                            | tee -a "$OUT"
echo ""                                               | tee -a "$OUT"

DATA="data/nyc_311/311_*.csv"

# ---- Phase 1 (Serial) ----
echo "=============================================" | tee -a "$OUT"
echo "  PHASE 1 — Serial (DataStore)"                | tee -a "$OUT"
echo "=============================================" | tee -a "$OUT"
./build/src/phase1/phase1 $DATA 2>&1 | tee -a "$OUT"
echo ""                                               | tee -a "$OUT"

# ---- Phase 2 (OpenMP) ----
export OMP_NUM_THREADS=$(nproc)
echo "=============================================" | tee -a "$OUT"
echo "  PHASE 2 — Parallel (ParallelDataStore)"      | tee -a "$OUT"
echo "  OMP_NUM_THREADS=$OMP_NUM_THREADS"             | tee -a "$OUT"
echo "=============================================" | tee -a "$OUT"
./build/src/phase2/phase2 $DATA 2>&1 | tee -a "$OUT"
echo ""                                               | tee -a "$OUT"

# ---- Phase 3 (SoA + SIMD + OpenMP) ----
echo "=============================================" | tee -a "$OUT"
echo "  PHASE 3 — Vectorized SoA (VectorStore)"      | tee -a "$OUT"
echo "  OMP_NUM_THREADS=$OMP_NUM_THREADS"             | tee -a "$OUT"
echo "=============================================" | tee -a "$OUT"
./build/src/phase3/phase3 $DATA 2>&1 | tee -a "$OUT"
echo ""                                               | tee -a "$OUT"

echo "=============================================" | tee -a "$OUT"
echo "  ALL DONE — $(date)"                           | tee -a "$OUT"
echo "  Results saved to: $OUT"                       | tee -a "$OUT"
echo "=============================================" | tee -a "$OUT"
