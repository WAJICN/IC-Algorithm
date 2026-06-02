#!/usr/bin/env python3
import json
import os
import sys

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.ticker import MaxNLocator, MultipleLocator


def set_integer_axis(ax, axis: str, values) -> None:
    span = max(values) - min(values) if values else 0
    locator = MultipleLocator(1) if span <= 30 else MaxNLocator(integer=True)
    if axis == "x":
        ax.xaxis.set_major_locator(locator)
    else:
        ax.yaxis.set_major_locator(locator)


def main() -> int:
    if len(sys.argv) != 3:
        print(f"usage: {sys.argv[0]} <solution.json> <output.png>", file=sys.stderr)
        return 1
    with open(sys.argv[1], "r", encoding="utf-8") as f:
        data = json.load(f)

    ops = data["operations"]
    colors = {"compute": "#2563eb", "memory": "#dc2626"}

    fig, ax = plt.subplots(figsize=(10, 4.5))
    for kind in sorted({op["kind"] for op in ops}):
        group = [op for op in ops if op["kind"] == kind]
        ax.scatter(
            [op["pos"] for op in group],
            [op["issue"] for op in group],
            s=70,
            color=colors.get(kind, "#525252"),
            edgecolor="white",
            linewidth=0.8,
            label=kind,
            zorder=3,
        )

    for op in ops:
        ax.vlines(op["pos"], op["issue"], op["finish"], color="#737373", linewidth=2)
        ax.text(
            op["pos"] + 0.08,
            op["issue"] + 0.08,
            op["id"],
            fontsize=8,
            va="bottom",
        )

    xs = [op["pos"] for op in ops]
    ys = [op["issue"] for op in ops] + [op["finish"] for op in ops]
    set_integer_axis(ax, "x", xs)
    set_integer_axis(ax, "y", ys)
    ax.set_xlim(min(xs) - 1, max(xs) + 1)
    ax.set_ylim(min(ys) - 1, max(ys) + 1)
    ax.set_xlabel("slice")
    ax.set_ylabel("issue cycle")
    ax.set_title("Placement and issue time")
    ax.legend(frameon=False, loc="best")
    ax.grid(alpha=0.25, linestyle="--")
    fig.tight_layout()
    os.makedirs(os.path.dirname(sys.argv[2]) or ".", exist_ok=True)
    fig.savefig(sys.argv[2], dpi=160)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
