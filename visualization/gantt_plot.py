#!/usr/bin/env python3
import json
import sys

import matplotlib.pyplot as plt


def main() -> int:
    if len(sys.argv) != 3:
        print(f"usage: {sys.argv[0]} <solution.json> <output.png>", file=sys.stderr)
        return 1
    with open(sys.argv[1], "r", encoding="utf-8") as f:
        data = json.load(f)

    ops = sorted(data["operations"], key=lambda op: (op["issue"], op["id"]))
    fig, ax = plt.subplots(figsize=(8, max(2, len(ops) * 0.35)))
    for row, op in enumerate(ops):
        ax.barh(row, max(1, op["dfunc"]), left=op["issue"], height=0.6)
        ax.text(op["issue"], row, f" {op['id']}@{op['pos']}", va="center", fontsize=8)
    ax.set_yticks(range(len(ops)))
    ax.set_yticklabels([op["id"] for op in ops])
    ax.set_xlabel("cycle")
    ax.set_title(f"DFG schedule, makespan={data['makespan']}")
    ax.grid(axis="x", alpha=0.3)
    fig.tight_layout()
    fig.savefig(sys.argv[2], dpi=160)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
