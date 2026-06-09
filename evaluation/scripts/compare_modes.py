#!/usr/bin/env python3
import csv
import json
import os
import subprocess
import sys
from pathlib import Path


def run_solve(solver: Path, dfg: Path, output: Path, mode: str, time_limit_ms: int) -> None:
    output.parent.mkdir(parents=True, exist_ok=True)
    if output.exists():
        output.unlink()
    completed = subprocess.run(
        [
            str(solver),
            str(dfg),
            str(output),
            "--mode",
            mode,
            "--time_limit_ms",
            str(time_limit_ms),
        ],
    )
    if completed.returncode != 0 and not output.exists():
        raise subprocess.CalledProcessError(completed.returncode, completed.args)


def load_row(dfg: Path, mode: str, output: Path) -> dict:
    with output.open("r", encoding="utf-8") as f:
        data = json.load(f)
    preprocess = data.get("preprocess", {})
    domain = data.get("domain_check", {})
    return {
        "workload": dfg.stem,
        "mode": mode,
        "status": data.get("status", ""),
        "makespan": data.get("makespan", ""),
        "variable_count": data.get("variable_count", ""),
        "constraint_count": data.get("constraint_count", ""),
        "solve_time_ms": data.get("solve_time_ms", ""),
        "preprocess_time_ms": data.get("preprocess_time_ms", ""),
        "total_op_pairs": preprocess.get("total_op_pairs", ""),
        "active_op_pairs": preprocess.get("active_op_pairs", ""),
        "pruned_op_pairs": preprocess.get("pruned_op_pairs", ""),
        "pair_reduction_rate": preprocess.get("pair_reduction_rate", ""),
        "average_issue_window": preprocess.get("average_issue_window", ""),
        "domain_check_ok": domain.get("ok", ""),
        "domain_check_errors": domain.get("error_count", ""),
    }


def main() -> int:
    repo = Path(__file__).resolve().parents[2]
    solver = repo / "build" / "solve"
    data_dir = repo / "evaluation" / "data"
    out_dir = repo / "evaluation" / "mode_comparison"
    csv_path = out_dir / "summary.csv"
    time_limit_ms = int(os.environ.get("TIME_LIMIT_MS", "90000"))

    if len(sys.argv) > 1:
        solver = Path(sys.argv[1])
    if not solver.exists():
        print(f"solver not found: {solver}", file=sys.stderr)
        return 1

    rows = []
    for dfg in sorted(data_dir.glob("*.dfg")):
        for mode in ("baseline", "graph"):
            output = out_dir / mode / f"{dfg.stem}.json"
            run_solve(solver, dfg, output, mode, time_limit_ms)
            rows.append(load_row(dfg, mode, output))

    csv_path.parent.mkdir(parents=True, exist_ok=True)
    with csv_path.open("w", encoding="utf-8", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=list(rows[0].keys()))
        writer.writeheader()
        writer.writerows(rows)

    print(f"wrote {csv_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
