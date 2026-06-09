#!/usr/bin/env python3
import csv
import json
import os
import subprocess
import sys
from pathlib import Path


def main() -> int:
    repo = Path(__file__).resolve().parents[2]
    solver = repo / "build" / "solve"
    if len(sys.argv) > 1:
        solver = Path(sys.argv[1])
    if not solver.exists():
        print(f"solver not found: {solver}", file=sys.stderr)
        return 1

    data_dir = repo / "evaluation" / "scalability" / "data"
    out_dir = repo / "evaluation" / "scalability" / "solutions"
    csv_path = repo / "evaluation" / "scalability" / "summary.csv"
    sizes = os.environ.get("SIZES", "25,50,75,100")
    time_limit_ms = int(os.environ.get("TIME_LIMIT_MS", "60000"))

    subprocess.run(
        [
            sys.executable,
            str(repo / "evaluation" / "scripts" / "generate_scalability_workloads.py"),
            "--sizes",
            sizes,
            "--out-dir",
            str(data_dir),
        ],
        check=True,
    )

    rows = []
    for dfg in sorted(data_dir.glob("scale_*.dfg")):
        output = out_dir / f"{dfg.stem}.json"
        output.parent.mkdir(parents=True, exist_ok=True)
        if output.exists():
            output.unlink()
        completed = subprocess.run(
            [
                str(solver),
                str(dfg),
                str(output),
                "--mode",
                "graph",
                "--time_limit_ms",
                str(time_limit_ms),
            ],
        )
        if completed.returncode != 0 and not output.exists():
            raise subprocess.CalledProcessError(completed.returncode, completed.args)
        with output.open("r", encoding="utf-8") as f:
            data = json.load(f)
        domain = data.get("domain_check", {})
        preprocess = data.get("preprocess", {})
        rows.append(
            {
                "workload": dfg.stem,
                "ops": sum(1 for line in dfg.read_text(encoding="utf-8").splitlines() if line.startswith("op ")),
                "status": data.get("status", ""),
                "makespan": data.get("makespan", ""),
                "variable_count": data.get("variable_count", ""),
                "constraint_count": data.get("constraint_count", ""),
                "solve_time_ms": data.get("solve_time_ms", ""),
                "pair_reduction_rate": preprocess.get("pair_reduction_rate", ""),
                "domain_check_ok": domain.get("ok", ""),
                "domain_check_errors": domain.get("error_count", ""),
            }
        )

    csv_path.parent.mkdir(parents=True, exist_ok=True)
    with csv_path.open("w", encoding="utf-8", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=list(rows[0].keys()))
        writer.writeheader()
        writer.writerows(rows)

    print(f"wrote {csv_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
