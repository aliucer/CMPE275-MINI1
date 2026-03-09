#!/usr/bin/env python3
"""Generates benchmark charts from hardcoded report data."""

import os
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np

# Hardcoded data from the benchmark report
PHASES = ["Phase 1 (Serial)", "Phase 2 (OpenMP)", "Phase 3 (SoA)"]
COLORS = ["#4C72B0", "#DD8452", "#55A868"]

LABELS = ["Q1_borough", "Q2_string", "Q3_geobox", "Q4_date", "Q5_centroid"]

# Mean latency in ms
DATA = [
    [150.2, 181.1, 223.9, 106.8, 94.8],  # Phase 1
    [77.6,  44.6,  77.3,  33.4,  27.7],  # Phase 2
    [68.5,  38.6,  81.7,  37.3,  8.6]    # Phase 3
]

# Load time and Memory
ENV_LABELS = ["Load Time (s)", "Memory (MB)"]
ENV_DATA = [
    [109.1, 2304], # Phase 1
    [24.3,  1400], # Phase 2
    [27.0,  1993]  # Phase 3
]

os.makedirs("plots", exist_ok=True)

def plot_query_comparison():
    x = np.arange(len(LABELS))
    w = 0.25
    fig, ax = plt.subplots(figsize=(10, 5))
    
    for i in range(3):
        ax.bar(x + (i - 1) * w, DATA[i], w, label=PHASES[i], color=COLORS[i])
        
    ax.set_xticks(x)
    ax.set_xticklabels([l.replace("_", "\n") for l in LABELS], fontsize=9)
    ax.set_ylabel("Mean query time (ms)")
    ax.set_title("Query Performance Comparison (Lower is Better)")
    ax.legend()
    ax.grid(axis="y", linestyle="--", alpha=0.4)
    fig.tight_layout()
    fig.savefig("plots/query_comparison.png", dpi=150)
    print("Saved plots/query_comparison.png")
    plt.close()

def plot_speedup():
    x = np.arange(len(LABELS))
    w = 0.35
    fig, ax = plt.subplots(figsize=(10, 5))
    
    for i in range(1, 3):
        speedups = [DATA[0][j] / DATA[i][j] for j in range(len(LABELS))]
        ax.bar(x + (i - 1.5) * w, speedups, w, label=PHASES[i], color=COLORS[i])
        
    ax.axhline(1, color="black", linestyle="--", linewidth=0.8)
    ax.set_xticks(x)
    ax.set_xticklabels([f"Q{j+1}" for j in range(len(LABELS))])
    ax.set_ylabel("Speedup vs Phase 1")
    ax.set_title("Speedup Over Serial Baseline (Higher is Better)")
    ax.legend()
    ax.grid(axis="y", linestyle="--", alpha=0.4)
    fig.tight_layout()
    fig.savefig("plots/speedup.png", dpi=150)
    print("Saved plots/speedup.png")
    plt.close()

def plot_load_memory():
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(10, 4))
    
    x = np.arange(3)
    
    # Load Time
    load_times = [d[0] for d in ENV_DATA]
    ax1.bar(x, load_times, color=COLORS)
    ax1.set_xticks(x)
    ax1.set_xticklabels([f"Phase {i+1}" for i in range(3)])
    ax1.set_ylabel("Seconds")
    ax1.set_title("CSV Load Time")
    ax1.grid(axis="y", linestyle="--", alpha=0.4)
    
    # Memory
    memory = [d[1] for d in ENV_DATA]
    ax2.bar(x, memory, color=COLORS)
    ax2.set_xticks(x)
    ax2.set_xticklabels([f"Phase {i+1}" for i in range(3)])
    ax2.set_ylabel("Megabytes (MB)")
    ax2.set_title("Peak Memory Usage")
    ax2.grid(axis="y", linestyle="--", alpha=0.4)
    
    fig.tight_layout()
    fig.savefig("plots/load_memory.png", dpi=150)
    print("Saved plots/load_memory.png")
    plt.close()

if __name__ == "__main__":
    plot_query_comparison()
    plot_speedup()
    plot_load_memory()
    print("Done.")
