#!/usr/bin/env python3
import csv
from pathlib import Path

import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
import numpy as np


ROOT = Path(__file__).resolve().parents[2]
OUT = Path(__file__).resolve().parent / "figures"
MODE_CSV = ROOT / "evaluation" / "mode_comparison" / "summary.csv"
SCALE_CSV = ROOT / "evaluation" / "scalability" / "summary.csv"

FONT_FAMILY = ["Arial", "DejaVu Sans", "sans-serif"]
BAR_FACE = "white"
BAR_EDGE = "black"
GRID_COLOR = "grey"
COLORS = ["#1f77b4", "#ff7f0e", "#2ca02c"]
HATCHES = ["///", "\\\\\\", "|||"]


def setup_style():
    plt.style.use("default")
    plt.rcParams.update(
        {
            "font.size": 17,
            "axes.labelsize": 20,
            "axes.titlesize": 18,
            "xtick.labelsize": 17,
            "ytick.labelsize": 17,
            "legend.fontsize": 16,
            "font.family": "sans-serif",
            "font.sans-serif": FONT_FAMILY,
            "hatch.linewidth": 1.0,
            "axes.linewidth": 1.4,
            "pdf.fonttype": 42,
            "ps.fonttype": 42,
        }
    )


def read_mode_rows():
    with MODE_CSV.open(newline="", encoding="utf-8") as f:
        rows = list(csv.DictReader(f))
    grouped = {}
    for row in rows:
        grouped.setdefault(row["workload"], {})[row["mode"]] = row
    solved = []
    for workload, modes in grouped.items():
        base = modes.get("baseline")
        graph = modes.get("graph")
        if (
            base
            and graph
            and base["status"] in ("optimal", "feasible")
            and graph["status"] in ("optimal", "feasible")
        ):
            solved.append((workload, base, graph))
    return solved


def short_name(workload):
    names = {
        "01_vector_pipeline": "Vec",
        "02_cnn_pipeline": "CNN",
        "03_residual_block": "Res",
        "05_transform_attention": "Attn",
        "06_memory_stream_mix": "Mem",
    }
    return names.get(workload, workload.removeprefix("0").split("_", 1)[-1])


def draw_bar(ax, x, values, labels, ylabel, ylim=None):
    width = 0.33
    handles = []
    for idx, (label, vals) in enumerate(zip(labels, values)):
        xpos = x + (idx - (len(values) - 1) / 2) * width
        ax.bar(xpos, vals, width, facecolor=BAR_FACE, edgecolor="none", zorder=1)
        ax.bar(
            xpos,
            vals,
            width,
            facecolor="none",
            edgecolor=COLORS[idx],
            hatch=HATCHES[idx],
            linewidth=0,
            zorder=2,
        )
        ax.bar(
            xpos,
            vals,
            width,
            facecolor="none",
            edgecolor=BAR_EDGE,
            linewidth=1.4,
            zorder=3,
        )
        handles.append(
            mpatches.Patch(
                facecolor=BAR_FACE,
                edgecolor=COLORS[idx],
                hatch=HATCHES[idx],
                label=label,
                linewidth=1.4,
            )
        )
    ax.set_ylabel(ylabel)
    if ylim:
        ax.set_ylim(ylim)
    ax.grid(True, axis="y", linestyle="--", color=GRID_COLOR, alpha=0.45, zorder=0)
    ax.tick_params(axis="both", direction="in", top=True, right=True, length=5)
    return handles


def save(fig, name, tight=True):
    OUT.mkdir(parents=True, exist_ok=True)
    if tight:
        fig.tight_layout(pad=0.4, w_pad=1.3, rect=(0, 0, 1, 0.88))
    fig.savefig(OUT / f"{name}.pdf", bbox_inches="tight")
    fig.savefig(OUT / f"{name}.png", dpi=240, bbox_inches="tight")
    plt.close(fig)


def plot_mode_comparison():
    rows = read_mode_rows()
    labels = [short_name(w) for w, _, _ in rows]
    x = np.arange(len(rows))
    base_constraints = np.array([int(b["constraint_count"]) for _, b, _ in rows])
    graph_constraints = np.array([int(g["constraint_count"]) for _, _, g in rows])
    speedups = np.array(
        [int(b["solve_time_ms"]) / max(1, int(g["solve_time_ms"])) for _, b, g in rows]
    )

    fig, axes = plt.subplots(1, 2, figsize=(8.8, 3.25))
    handles = draw_bar(
        axes[0],
        x,
        [base_constraints / 1000.0, graph_constraints / 1000.0],
        ["Baseline", "Graph"],
        "Constraints (K)",
    )
    axes[0].set_xticks(x)
    axes[0].set_xticklabels(labels)
    axes[0].legend(
        handles=handles,
        loc="lower center",
        bbox_to_anchor=(0.5, 1.02),
        ncol=2,
        frameon=False,
        handlelength=1.8,
        columnspacing=1.0,
    )

    axes[1].bar(x, speedups, 0.45, facecolor=BAR_FACE, edgecolor="none", zorder=1)
    axes[1].bar(
        x,
        speedups,
        0.45,
        facecolor="none",
        edgecolor=COLORS[2],
        hatch=HATCHES[2],
        linewidth=0,
        zorder=2,
    )
    axes[1].bar(
        x,
        speedups,
        0.45,
        facecolor="none",
        edgecolor=BAR_EDGE,
        linewidth=1.4,
        zorder=3,
    )
    axes[1].axhline(1.0, color="grey", linestyle="--", linewidth=1.3)
    axes[1].set_ylabel("Speedup")
    axes[1].set_xticks(x)
    axes[1].set_xticklabels(labels)
    axes[1].set_ylim(0, max(3.8, speedups.max() * 1.18))
    axes[1].grid(True, axis="y", linestyle="--", color=GRID_COLOR, alpha=0.45, zorder=0)
    axes[1].tick_params(axis="both", direction="in", top=True, right=True, length=5)
    save(fig, "mode_comparison")


def plot_scalability():
    with SCALE_CSV.open(newline="", encoding="utf-8") as f:
        rows = list(csv.DictReader(f))
    ops = np.array([int(r["ops"]) for r in rows])
    solve_ms = np.array([int(r["solve_time_ms"]) for r in rows]) / 1000.0
    constraints = np.array([int(r["constraint_count"]) for r in rows]) / 1000.0
    pair_reduction = np.array([float(r["pair_reduction_rate"]) for r in rows]) * 100.0

    fig, axes = plt.subplots(1, 2, figsize=(8.8, 3.25))
    axes[0].plot(ops, solve_ms, color="black", marker="o", linewidth=1.8, markersize=6)
    axes[0].set_xlabel("Operations")
    axes[0].set_ylabel("Solve time (s)")
    axes[0].set_ylim(0, 68)
    axes[0].grid(True, axis="y", linestyle="--", color=GRID_COLOR, alpha=0.45)
    axes[0].tick_params(axis="both", direction="in", top=True, right=True, length=5)

    handles = draw_bar(
        axes[1],
        np.arange(len(rows)),
        [constraints, pair_reduction],
        ["Constraints (K)", "Pair reduction (%)"],
        "Value",
    )
    axes[1].set_xticks(np.arange(len(rows)))
    axes[1].set_xticklabels([str(v) for v in ops])
    axes[1].set_xlabel("Operations")
    axes[1].legend(
        handles=handles,
        loc="lower center",
        bbox_to_anchor=(0.5, 1.02),
        ncol=2,
        frameon=False,
        handlelength=1.8,
        columnspacing=1.0,
    )
    save(fig, "scalability")


def plot_results_overview():
    mode_rows = read_mode_rows()
    mode_labels = [short_name(w) for w, _, _ in mode_rows]
    mode_x = np.arange(len(mode_rows))
    base_constraints = np.array([int(b["constraint_count"]) for _, b, _ in mode_rows])
    graph_constraints = np.array([int(g["constraint_count"]) for _, _, g in mode_rows])
    speedups = np.array(
        [int(b["solve_time_ms"]) / max(1, int(g["solve_time_ms"])) for _, b, g in mode_rows]
    )

    with SCALE_CSV.open(newline="", encoding="utf-8") as f:
        scale_rows = list(csv.DictReader(f))
    ops = np.array([int(r["ops"]) for r in scale_rows])
    solve_s = np.array([int(r["solve_time_ms"]) for r in scale_rows]) / 1000.0
    scale_constraints = np.array([int(r["constraint_count"]) for r in scale_rows]) / 1000.0
    pair_reduction = np.array([float(r["pair_reduction_rate"]) for r in scale_rows]) * 100.0

    fig, axes = plt.subplots(2, 2, figsize=(10.8, 7.4))
    bar_handles = draw_bar(
        axes[0, 0],
        mode_x,
        [base_constraints / 1000.0, graph_constraints / 1000.0],
        ["Baseline", "Graph"],
        "Constraints (K)",
    )
    axes[0, 0].set_xticks(mode_x)
    axes[0, 0].set_xticklabels(mode_labels)
    axes[0, 0].set_title("(a) Existing workload model size", pad=8)

    axes[0, 1].bar(mode_x, speedups, 0.5, facecolor=BAR_FACE, edgecolor="none", zorder=1)
    axes[0, 1].bar(
        mode_x,
        speedups,
        0.5,
        facecolor="none",
        edgecolor=COLORS[2],
        hatch=HATCHES[2],
        linewidth=0,
        zorder=2,
    )
    axes[0, 1].bar(
        mode_x,
        speedups,
        0.5,
        facecolor="none",
        edgecolor=BAR_EDGE,
        linewidth=1.4,
        zorder=3,
    )
    axes[0, 1].axhline(1.0, color="grey", linestyle="--", linewidth=1.3)
    axes[0, 1].set_ylabel("Speedup")
    axes[0, 1].set_xticks(mode_x)
    axes[0, 1].set_xticklabels(mode_labels)
    axes[0, 1].set_ylim(0, max(3.8, speedups.max() * 1.18))
    axes[0, 1].grid(True, axis="y", linestyle="--", color=GRID_COLOR, alpha=0.45, zorder=0)
    axes[0, 1].tick_params(axis="both", direction="in", top=True, right=True, length=5)
    axes[0, 1].set_title("(b) Existing workload solve time", pad=8)

    axes[1, 0].plot(ops, solve_s, color="black", marker="o", linewidth=1.8, markersize=6)
    axes[1, 0].set_xlabel("Operations")
    axes[1, 0].set_ylabel("Solve time (s)")
    axes[1, 0].set_ylim(0, 68)
    axes[1, 0].grid(True, axis="y", linestyle="--", color=GRID_COLOR, alpha=0.45)
    axes[1, 0].tick_params(axis="both", direction="in", top=True, right=True, length=5)
    axes[1, 0].set_title("(c) Scalability solve time", pad=8)

    scale_handles = draw_bar(
        axes[1, 1],
        np.arange(len(scale_rows)),
        [scale_constraints, pair_reduction],
        ["Constraints (K)", "Pair reduction (%)"],
        "Value",
    )
    axes[1, 1].set_xticks(np.arange(len(scale_rows)))
    axes[1, 1].set_xticklabels([str(v) for v in ops])
    axes[1, 1].set_xlabel("Operations")
    axes[1, 1].set_title("(d) Scalability model size", pad=8)
    fig.legend(
        handles=bar_handles,
        loc="upper center",
        bbox_to_anchor=(0.29, 0.99),
        ncol=2,
        frameon=False,
        handlelength=1.8,
        columnspacing=1.0,
    )
    fig.legend(
        handles=scale_handles,
        loc="upper center",
        bbox_to_anchor=(0.74, 0.99),
        ncol=2,
        frameon=False,
        handlelength=1.8,
        columnspacing=1.0,
    )
    fig.subplots_adjust(
        left=0.075,
        right=0.985,
        bottom=0.075,
        top=0.85,
        hspace=0.62,
        wspace=0.30,
    )
    save(fig, "results_overview", tight=False)


def main():
    setup_style()
    plot_mode_comparison()
    plot_scalability()
    plot_results_overview()
    print(f"wrote figures to {OUT}")


if __name__ == "__main__":
    main()
