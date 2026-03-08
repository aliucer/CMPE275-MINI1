#!/usr/bin/env python3
"""Reads phaseN_results.csv files and generates comparison charts."""

import os, csv
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np

PHASES = [
    ("phase1_results.csv", "Phase 1 (Serial)"),
    ("phase2_results.csv", "Phase 2 (OpenMP)"),
    ("phase3_results.csv", "Phase 3 (SoA)"),
]
COLORS = ["#4C72B0", "#DD8452", "#55A868"]

os.makedirs("plots", exist_ok=True)


def read_csv(path):
    if not os.path.exists(path):
        return {}
    with open(path) as f:
        return {row["label"]: float(row["mean_ms"]) for row in csv.DictReader(f)}


def main():
    data = [read_csv(f) for f, _ in PHASES]
    labels = list(data[0].keys()) if data[0] else []
    if not labels:
        print("No data found")
        return

    # query comparison chart
    x = np.arange(len(labels))
    w = 0.25
    fig, ax = plt.subplots(figsize=(10, 5))
    for i, (d, color, (_, name)) in enumerate(zip(data, COLORS, PHASES)):
        vals = [d.get(l, 0) for l in labels]
        ax.bar(x + (i - 1) * w, vals, w, label=name, color=color)
    ax.set_xticks(x)
    ax.set_xticklabels([l.replace("_", "\n") for l in labels], fontsize=8)
    ax.set_ylabel("Mean query time (ms)")
    ax.set_title("Query Performance Comparison")
    ax.legend()
    ax.grid(axis="y", linestyle="--", alpha=0.4)
    fig.tight_layout()
    fig.savefig("plots/query_comparison.png", dpi=150)
    print("Saved plots/query_comparison.png")
    plt.close()

    # speedup chart
    if data[0]:
        fig, ax = plt.subplots(figsize=(10, 5))
        for i, (d, color, (_, name)) in enumerate(zip(data[1:], COLORS[1:], PHASES[1:])):
            speedups = [data[0][l] / d[l] if d.get(l, 0) > 0 else 0 for l in labels]
            ax.bar(x + (i - 0.5) * 0.35, speedups, 0.35, label=name, color=color)
        ax.axhline(1, color="black", linestyle="--", linewidth=0.8)
        ax.set_xticks(x)
        ax.set_xticklabels([f"Q{i+1}" for i in range(len(labels))])
        ax.set_ylabel("Speedup vs Phase 1")
        ax.set_title("Speedup Over Serial Baseline")
        ax.legend()
        ax.grid(axis="y", linestyle="--", alpha=0.4)
        fig.tight_layout()
        fig.savefig("plots/speedup.png", dpi=150)
        print("Saved plots/speedup.png")
        plt.close()

    print("Done.")


if __name__ == "__main__":
    main()
