#!/usr/bin/env python3
import argparse
from pathlib import Path


def chain_length(size: int) -> int:
    return min(size, 50)


def slice_spec(index: int, size: int) -> str:
    chain_ops = chain_length(size)
    if index == 0 or index == chain_ops - 1:
        return "-1,1"
    return "0"


def op_kind(index: int, size: int) -> str:
    chain_ops = chain_length(size)
    if index == 0 or index == chain_ops - 1:
        return "memory"
    return "compute"


def dfunc(index: int, size: int) -> int:
    chain_ops = chain_length(size)
    if index == 0 or index == chain_ops - 1:
        return 1
    return 1 + (index % 3)


def write_workload(path: Path, size: int) -> None:
    lines = [
        f"# Generated scalability workload with {size} operations.",
        "# First operations form a timing-valid pipeline; remaining ops are independent.",
    ]
    for i in range(size):
        lines.append(
            f"op n{i:03d} {op_kind(i, size)} {dfunc(i, size)} "
            f"slices={slice_spec(i, size)}"
        )

    for i in range(chain_length(size) - 1):
        lines.append(f"edge n{i:03d} n{i + 1:03d}")

    path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--sizes", default="25,50,75,100")
    parser.add_argument(
        "--out-dir",
        default=str(Path(__file__).resolve().parents[1] / "scalability" / "data"),
    )
    args = parser.parse_args()

    out_dir = Path(args.out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)
    for raw_size in args.sizes.split(","):
        size = int(raw_size)
        if size < 2:
            raise ValueError("size must be at least 2")
        write_workload(out_dir / f"scale_{size:03d}.dfg", size)

    print(f"wrote workloads to {out_dir}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
