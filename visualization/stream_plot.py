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


def route_points(edge, src, dst):
    step = 1 if dst["pos"] >= src["pos"] else -1
    if src["pos"] == dst["pos"]:
        xs = [src["pos"]]
    else:
        xs = list(range(src["pos"], dst["pos"] + step, step))

    if edge["direction"] == "east":
        ys = [edge["phase"] + x for x in xs]
    else:
        ys = [edge["phase"] - x for x in xs]
    return xs, ys


def main() -> int:
    if len(sys.argv) != 3:
        print(f"usage: {sys.argv[0]} <solution.json> <output.png>", file=sys.stderr)
        return 1
    with open(sys.argv[1], "r", encoding="utf-8") as f:
        data = json.load(f)

    ops = data["operations"]
    op_by_id = {op["id"]: op for op in ops}
    edges = data["edges"]

    fig, ax = plt.subplots(figsize=(10, 5.5))
    cmap = plt.get_cmap("tab20")

    for op in ops:
        ax.vlines(op["pos"], op["issue"], op["finish"], color="#404040", linewidth=3)
        ax.scatter([op["pos"]], [op["issue"]], color="#404040", s=24, zorder=4)
        ax.scatter([op["pos"]], [op["finish"]], color="white", edgecolor="#404040", s=28, zorder=4)
        ax.text(op["pos"] + 0.08, op["issue"] + 0.08, op["id"], fontsize=8)

    all_x = [op["pos"] for op in ops]
    all_y = [op["issue"] for op in ops] + [op["finish"] for op in ops]
    for index, edge in enumerate(edges):
        src = op_by_id[edge["src"]]
        dst = op_by_id[edge["dst"]]
        xs, ys = route_points(edge, src, dst)
        color = cmap(index % cmap.N)
        label = f"{edge['src']}->{edge['dst']}  s{edge['stream']}"
        ax.plot(
            xs,
            ys,
            color=color,
            linewidth=2.5,
            marker="o",
            markersize=4,
            label=label,
            zorder=3,
        )

        route_arrival = ys[-1]
        if dst["issue"] > route_arrival:
            ax.plot(
                [dst["pos"], dst["pos"]],
                [route_arrival, dst["issue"]],
                color=color,
                linewidth=1.4,
                linestyle=":",
                alpha=0.8,
            )

        mid = len(xs) // 2
        ax.text(
            xs[mid] + 0.08,
            ys[mid] + 0.08,
            f"s{edge['stream']}",
            color=color,
            fontsize=8,
        )

        all_x.extend(xs)
        all_y.extend(ys)
        all_y.append(dst["issue"])

    set_integer_axis(ax, "x", all_x)
    set_integer_axis(ax, "y", all_y)
    ax.set_xlim(min(all_x) - 1, max(all_x) + 1)
    ax.set_ylim(min(all_y) - 1, max(all_y) + 1)
    ax.set_xlabel("slice")
    ax.set_ylabel("cycle")
    ax.set_title("Stream route paths")
    ax.grid(alpha=0.25, linestyle="--")
    ax.legend(frameon=False, fontsize=8, loc="best")
    fig.tight_layout()
    os.makedirs(os.path.dirname(sys.argv[2]) or ".", exist_ok=True)
    fig.savefig(sys.argv[2], dpi=160)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
