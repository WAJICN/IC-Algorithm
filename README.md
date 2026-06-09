# DFG Placement and Scheduling ILP

This project models a DFG placement and scheduling problem for the course
project of Integrated Circuit Engineering Algorithms.

The solver uses Google OR-Tools C++ with a mixed-integer linear formulation.
Given a DFG, it decides:

- `issue(i)`: the issue cycle of each operation.
- `pos(i)`: the physical slice assigned to each operation.
- stream direction and stream ID for each DFG edge.

The hardware model is fixed in `model/hardware.h`: slices `[-46, 46]`,
32 streams, stream latency `KStream`, and per-slice ICU capacity.

## Build

```bash
cmake -S . -B build
cmake --build build
```

The default `ORTOOLS_ROOT` is:

```text
../or-tools_x86_64_debian-sid_cpp_v9.10.4067
```

Override it if needed:

```bash
cmake -S . -B build -DORTOOLS_ROOT=/path/to/or-tools
```

## Run

```bash
./build/solve examples/simple_chain.dfg outputs/solutions/simple_chain.json
```

Optional solver flags:

```bash
./build/solve examples/simple_chain.dfg outputs/solutions/simple_chain.json \
  --time_limit_ms 10000 --solver SCIP
```

Select the solver mode:

```bash
# Direct ILP baseline. This is the default.
./build/solve examples/simple_chain.dfg outputs/baseline/simple_chain.json \
  --mode baseline

# Graph-preprocessed ILP with ASAP/ALAP bounds and incomparable-pair pruning.
./build/solve examples/simple_chain.dfg outputs/graph/simple_chain.json \
  --mode graph
```

Compare both modes on the evaluation workloads:

```bash
python3 evaluation/scripts/compare_modes.py
```

The comparison script writes per-mode JSON files and a CSV summary under
`evaluation/mode_comparison/`.

Generate and run scalability workloads in graph-preprocessed mode:

```bash
python3 evaluation/scripts/run_scalability.py
```

Use `SIZES=25,50,75,100` and `TIME_LIMIT_MS=60000` environment variables to
control the scalability run.

## DFG Format

See `docs/dfg_format.md`.
