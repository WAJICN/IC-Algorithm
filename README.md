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

## DFG Format

See `docs/dfg_format.md`.
