# DFG Placement and Scheduling ILP Project

## 1. Project Goal

This repository implements a course-project prototype for **DFG placement and scheduling** on a simplified spatial hardware model.

The input is a directed data-flow graph, or DFG. Each node is an operation, such as `conv1`, `relu1`, `mul_x0_w0`, or `write_reduce_sum`. Each edge is a producer-to-consumer dependency. The solver decides:

- **where** to place each operation on a physical slice;
- **when** to issue each operation;
- which stream direction and stream ID to use for each routed data dependency.

The optimization backend is **Google OR-Tools C++**, using the `MPSolver` mixed-integer programming interface with the local OR-Tools package:

```text
../or-tools_x86_64_debian-sid_cpp_v9.10.4067
```

The objective is to minimize the whole graph finish time, represented by a `makespan` variable.

## 2. Repository Structure

```text
.
├── CMakeLists.txt
├── README.md
├── dfg/
│   ├── dfg.h / dfg.cpp
│   ├── parser.h / parser.cpp
│   └── validator.h / validator.cpp
├── model/
│   ├── hardware.h
│   ├── ilp_solver.h / ilp_solver.cpp
│   ├── variables.h / variables.cpp
│   ├── objective.h / objective.cpp
│   ├── solution.h / solution.cpp
│   └── constraints/
│       ├── legal_slice.h / legal_slice.cpp
│       ├── slice_icu.h / slice_icu.cpp
│       ├── stream.h / stream.cpp
│       └── timing.h / timing.cpp
├── visualization/
│   ├── exporter.h / exporter.cpp
│   ├── schedule_route_plot.py
│   ├── gantt_plot.py
│   ├── placement_plot.py
│   └── stream_plot.py
├── tools/
│   └── solve.cpp
├── examples/
│   ├── simple_chain.dfg
│   ├── diamond.dfg
│   ├── mem_compute_mix.dfg
│   ├── stream_conflict.dfg
│   ├── cnn_3layer.dfg
│   ├── resnet_tiny_block.dfg
│   └── binary_tree_reduce.dfg
├── evaluation/
│   ├── data/
│   │   ├── 01_vector_pipeline.dfg
│   │   ├── 02_cnn_pipeline.dfg
│   │   ├── 03_residual_block.dfg
│   │   ├── 04_binary_tree_reduce.dfg
│   │   ├── 05_transform_attention.dfg
│   │   └── 06_memory_stream_mix.dfg
│   ├── solutions/
│   │   └── *.json
│   └── figures/
│       └── *.svg
├── tests/
│   ├── test_dfg_parser.cpp
│   ├── test_dfg_validator.cpp
│   └── test_small_solve.cpp
└── docs/
    ├── dfg_format.md
    ├── formulation.md
    └── document.md
```

The implementation is intentionally split into layers:

- `dfg/`: parse and validate the input graph.
- `model/`: build and solve the ILP model.
- `model/constraints/`: keep each constraint family in a separate file.
- `visualization/`: export and draw solution results.
- `examples/`: DFG instances for testing and reporting.
- `evaluation/data/`: representative final-evaluation DFG workloads.
- `evaluation/solutions/`: JSON solver outputs for final-evaluation workloads.
- `evaluation/figures/`: SVG schedule-placement-route figures for final-evaluation workloads.
- `tests/`: small executable tests.

## 3. DFG Input Format

The parser uses a small line-oriented text format. Blank lines and comments starting with `#` are ignored.

An operation is written as:

```text
op <id> <kind> <dfunc> [slices=<slice-spec>]
```

Example:

```text
op conv1 compute 3 slices=-46,46
op relu1 compute 1 slices=0
op input_read memory 1 slices=-44..-1,1..44
```

Fields:

- `id`: unique operation ID.
- `kind`: `compute`, `comp`, `memory`, or `mem`.
- `dfunc`: operation latency in cycles.
- `slices`: feasible physical slice set. This can use integer lists or ranges.

An edge is written as:

```text
edge <src-id> <dst-id>
```

Example:

```text
edge conv1 bias_add1
edge bias_add1 relu1
```

The parser validates syntax and references. The validator also checks:

- graph is non-empty;
- operation IDs are unique;
- edges reference known operations;
- no duplicate edges;
- no self edges;
- the graph is acyclic.

## 4. Hardware Model

The hardware model is fixed in `model/hardware.h`.

Current constants:

```text
slice range:      -46 ... 46
stream count:     32
KStream:          1
ICU capacity:     1 per legal slice
max issue time:   2000
big-M:            10000
```

The examples follow these operation-class placement rules:

```text
matrix ops:     slices=-46,46
vector ops:     slices=0
transform ops:  slices=-45,45
memory ops:     slices=-44..-1,1..44
```

In the current examples:

- matrix-like operations include `conv`, `matmul`, and `mul`;
- vector-like operations include `relu`, `bias_add`, `batch_norm`, `pool`, and `add`;
- memory operations use names such as `read_*` and `write_*`.

The solver does not infer these classes from operation names. The legal slices are encoded directly in each `.dfg` file through the `slices=` attribute, so a custom operation such as `skip_projection` follows whichever domain the DFG author assigns.

## 5. ILP Decision Variables

The core decision variables are:

```text
issue(i)  integer issue cycle of operation i
pos(i)    integer physical slice of operation i
y(i,p)    binary variable: operation i is placed on slice p
makespan  integer finish time upper bound
```

The model also creates helper variables for pairwise operation constraints:

```text
abs_pos_delta(i,j)      |pos(i) - pos(j)|
abs_issue_delta(i,j)    |issue(i) - issue(j)|
same_slice_cycle(i,j)   helper for slice ICU capacity
```

For each DFG edge, stream variables are created:

```text
east(e), west(e)        stream direction
stream(e,s)             edge e chooses stream s
lo(e), hi(e)            route interval endpoints
phase(e)                route timing phase
```

For each pair of edges, more helper variables describe whether the two routes:

- use the same direction;
- use the same stream ID;
- overlap in slice interval;
- need phase separation.

## 6. Objective

The objective is to minimize the graph makespan:

```text
minimize makespan
```

For each operation:

```text
makespan >= issue(i) + dfunc(i)
```

This turns the original “minimize the finish time of the whole compute graph” idea into a standard linear objective.

## 7. Constraint Families

### 7.1 Legal Slice Constraints

Implemented in:

```text
model/constraints/legal_slice.cpp
```

Each operation must choose exactly one legal slice:

```text
sum_p y(i,p) = 1
```

The physical position is linked to the selected slice:

```text
pos(i) = sum_p p * y(i,p)
```

Here, `p` iterates only over the feasible slice set given in the DFG input.

### 7.2 Timing Constraints

Implemented in:

```text
model/constraints/timing.cpp
```

For a dependency edge `i -> j`, the route latency depends on physical distance:

```text
distance(i,j) = |pos(i) - pos(j)|
```

If either endpoint is a compute operation, the solver applies exact timing:

```text
issue(i) + dfunc(i) + distance(i,j) + KStream = issue(j)
```

If both endpoints are memory operations, the solver applies a lower-bound dependency:

```text
issue(i) + dfunc(i) + distance(i,j) + KStream <= issue(j)
```

This distinction follows the original formulation.

### 7.3 Slice ICU Constraints

Implemented in:

```text
model/constraints/slice_icu.cpp
```

The model prevents two operations from using the same slice at the same issue cycle when the ICU capacity is one.

The implementation introduces exact absolute-value linearizations for:

```text
|pos(i) - pos(j)|
|issue(i) - issue(j)|
```

Then it enforces the no-conflict relation:

```text
abs_pos_delta(i,j) + abs_issue_delta(i,j) + M * same_slice_cycle(i,j) >= 1
```

The per-operation ICU capacity is then constrained using the selected slice capacity:

```text
sum_j same_slice_cycle(i,j) <= ICU(pos(i)) - 1
```

The current hardware uses `ICU(pos) = 1` for every legal slice.

### 7.4 Stream Constraints

Implemented in:

```text
model/constraints/stream.cpp
```

Each edge chooses one direction:

```text
east(e) + west(e) = 1
```

Each edge chooses one stream ID:

```text
sum_s stream(e,s) = 1
```

The route interval is represented by:

```text
lo(e) = min(pos(src), pos(dst))
hi(e) = max(pos(src), pos(dst))
```

The phase variable represents when data enters a directional stream:

```text
east: phase(e) = issue(src) + dfunc(src) - pos(src)
west: phase(e) = issue(src) + dfunc(src) + pos(src)
```

For every pair of routes, the model computes whether they:

- choose the same stream ID;
- choose the same direction;
- overlap in physical slice interval.

If all three are true, the phases must be separated by at least one cycle. This prevents two overlapping routes from using the same stream in the same direction at the same phase.

## 8. Solver Flow

The CLI entry point is:

```text
tools/solve.cpp
```

The top-level solver wrapper is:

```text
model/ilp_solver.cpp
```

The flow is:

1. Parse `.dfg` file.
2. Validate the graph.
3. Create an OR-Tools `MPSolver`.
4. Create all decision and helper variables.
5. Add legal-slice constraints.
6. Add slice ICU constraints.
7. Add timing constraints.
8. Add stream constraints.
9. Add makespan objective.
10. Solve.
11. Verify the solution through OR-Tools.
12. Export a JSON solution.

The JSON solution contains:

```text
status
objective_value
makespan
variable_count
constraint_count
operations
edges
```

Each operation entry contains:

```text
id, kind, dfunc, issue, finish, pos
```

Each edge entry contains:

```text
src, dst, stream, direction, lo, hi, phase
```

## 9. Build and Run

Configure and build:

```bash
cmake -S . -B build
cmake --build build
```

Run one example:

```bash
./build/solve examples/cnn_3layer.dfg outputs/solutions/cnn_3layer.json --time_limit_ms 90000
```

Run one final-evaluation workload and save it in the evaluation output folders:

```bash
./build/solve evaluation/data/02_cnn_pipeline.dfg evaluation/solutions/02_cnn_pipeline.json --time_limit_ms 90000
python3 visualization/schedule_route_plot.py \
  evaluation/solutions/02_cnn_pipeline.json \
  evaluation/figures/02_cnn_pipeline.svg
```

Run the direct ILP baseline explicitly:

```bash
./build/solve evaluation/data/02_cnn_pipeline.dfg \
  outputs/baseline/02_cnn_pipeline.json \
  --mode baseline --time_limit_ms 90000
```

Run the graph-preprocessed ILP mode:

```bash
./build/solve evaluation/data/02_cnn_pipeline.dfg \
  outputs/graph/02_cnn_pipeline.json \
  --mode graph --time_limit_ms 90000
```

Run both modes for all evaluation workloads and write a CSV summary:

```bash
python3 evaluation/scripts/compare_modes.py
```

The comparison script writes:

```text
evaluation/mode_comparison/baseline/*.json
evaluation/mode_comparison/graph/*.json
evaluation/mode_comparison/summary.csv
```

Run the generated scalability evaluation:

```bash
python3 evaluation/scripts/run_scalability.py
```

The scalability run writes generated DFGs, graph-mode solutions, and a summary:

```text
evaluation/scalability/data/*.dfg
evaluation/scalability/solutions/*.json
evaluation/scalability/summary.csv
```

The default scalability sizes are `25,50,75,100` operations and the default time
limit is 60000 ms. Override them with:

```bash
SIZES=25,50,75,100 TIME_LIMIT_MS=60000 \
  python3 evaluation/scripts/run_scalability.py
```

Run tests:

```bash
ctest --test-dir build --output-on-failure
```

The OR-Tools root defaults to:

```text
../or-tools_x86_64_debian-sid_cpp_v9.10.4067
```

It can be overridden:

```bash
cmake -S . -B build -DORTOOLS_ROOT=/path/to/or-tools
```

## 10. Examples

The repository currently includes seven example DFGs.

### 10.1 `simple_chain.dfg`

A minimal memory-vector-memory chain:

```text
read_x -> relu_x -> write_x
```

This is useful for smoke testing parser, timing constraints, legal slices, and JSON export.

### 10.2 `mem_compute_mix.dfg`

Mixes memory-to-memory lower-bound timing and memory-to-compute exact timing:

```text
read_activation0 -> read_activation1 -> vec_add -> write_activation
```

### 10.3 `stream_conflict.dfg`

Creates two independent routes:

```text
matmul_q -> relu_q
read_k -> write_k
```

This is useful for checking stream assignment and route plotting.

### 10.4 `diamond.dfg`

A fan-out/fan-in graph:

```text
read_input -> relu_input
relu_input -> conv_left
relu_input -> conv_right
conv_left -> vec_add_join
conv_right -> vec_add_join
```

The two convolution branches are placed on opposite matrix slices.

### 10.5 `cnn_3layer.dfg`

A small CNN-like pipeline:

```text
input_read -> conv1 -> bias_add1 -> relu1 -> maxpool1
maxpool1 -> conv2 -> bias_add2 -> relu2 -> batch_norm2
batch_norm2 -> conv3 -> bias_add3 -> relu3 -> global_avg_pool -> fc_matmul -> logits_write
```

It demonstrates repeated matrix/vector transitions.

### 10.6 `resnet_tiny_block.dfg`

A tiny residual block:

```text
input_read -> conv1 -> batch_norm1 -> relu1 -> conv2 -> batch_norm2
input_read -> skip_projection
batch_norm2 -> residual_add
skip_projection -> residual_add
residual_add -> relu_out -> output_write
```

This example tests a join with a skip branch.

### 10.7 `binary_tree_reduce.dfg`

An eight-input binary tree reduction:

```text
read_x* -> mul_x*_w*
mul outputs -> add_pair_*
add_pair outputs -> add_quad_*
add_quad outputs -> relu_reduce_sum -> write_reduce_sum
```

This is the largest included example and creates many route-pair constraints.

## 11. Final Evaluation Workloads

The final-evaluation workloads are stored under:

```text
evaluation/data/
```

They are designed to cover representative DFG shapes that are useful for evaluating placement, timing, resource, and stream-routing behavior.

The generated solver outputs are stored under:

```text
evaluation/solutions/
```

The generated figures are stored under:

```text
evaluation/figures/
```

Current final-evaluation workloads:

```text
01_vector_pipeline.dfg
02_cnn_pipeline.dfg
03_residual_block.dfg
04_binary_tree_reduce.dfg
05_transform_attention.dfg
06_memory_stream_mix.dfg
```

### 11.1 `01_vector_pipeline.dfg`

A compact vector-processing pipeline with memory reads, vector arithmetic, activation, and output write. This workload is useful as a small baseline because it exercises the full parse-solve-export-visualize flow without creating many pairwise stream constraints.

### 11.2 `02_cnn_pipeline.dfg`

A multi-layer CNN-style graph with repeated memory, convolution, bias, activation, pooling, and final matrix multiplication stages. This workload emphasizes matrix/vector transitions and long-distance routes between matrix slices and the vector slice.

### 11.3 `03_residual_block.dfg`

A residual block with a main convolution path and a skip-projection path that join at an add operation. This workload tests branch synchronization, exact timing constraints, and stream assignment around a fan-in point.

### 11.4 `04_binary_tree_reduce.dfg`

A binary-tree reduction with many independent input reads, multiplication operations, pairwise adds, higher-level adds, activation, and a final write. This workload creates many operation pairs and route pairs, so it is useful for stress-testing pairwise resource and stream constraints.

### 11.5 `05_transform_attention.dfg`

A transformer/attention-inspired workload with memory reads, matrix operations, transform operations such as transpose/replace, vector operations, and output write. This workload specifically exercises the transform slice domain `-45,45`.

### 11.6 `06_memory_stream_mix.dfg`

A mixed memory and compute workload with several memory paths and compute joins. This workload is useful for checking memory-to-memory lower-bound timing, memory-to-compute exact timing, and stream-route visualization.

All evaluation DFGs follow the same operation-class placement convention:

```text
matrix ops:     slices=-46,46
vector ops:     slices=0
transform ops:  slices=-45,45
memory ops:     slices=-44..-1,1..44
```

The current generated solutions are:

```text
01_vector_pipeline.json       optimal, makespan 14
02_cnn_pipeline.json          optimal, makespan 320
03_residual_block.json        optimal, makespan 164
04_binary_tree_reduce.json    optimal, makespan 66
05_transform_attention.json   optimal, makespan 184
06_memory_stream_mix.json     optimal, makespan 111
```

The matching SVG figures are:

```text
01_vector_pipeline.svg
02_cnn_pipeline.svg
03_residual_block.svg
04_binary_tree_reduce.svg
05_transform_attention.svg
06_memory_stream_mix.svg
```

## 12. Visualization

The main visualization script is:

```text
visualization/schedule_route_plot.py
```

It reads a solution JSON and writes an SVG figure.

Example:

```bash
python3 visualization/schedule_route_plot.py \
  evaluation/solutions/02_cnn_pipeline.json \
  evaluation/figures/02_cnn_pipeline.svg
```

The combined figure uses:

- x-axis: cycle;
- y-axis: used physical positions;
- colored bars: operation duration from issue to finish;
- dashed colored lines: routed data dependencies;
- compact edge labels such as `e1`, `e2`, etc.;
- SVG tooltips containing full edge names and stream IDs;
- SVG tooltips on operation bars containing issue time, finish time, and position.

The y-axis intentionally shows only positions used by operations. Empty slices are omitted so that figures remain readable when operations are placed at sparse locations such as `-46`, `0`, and `46`.

Other plotting scripts are also present:

```text
visualization/gantt_plot.py
visualization/placement_plot.py
visualization/stream_plot.py
```

Those scripts use `matplotlib`. The combined SVG script does not require external Python packages.

## 13. Tests

The test suite currently includes:

```text
test_dfg_parser
test_dfg_validator
test_small_solve
```

They check:

- parser behavior;
- cycle detection in the validator;
- a small end-to-end solve;
- route interval and phase consistency in the extracted solution.

Run:

```bash
ctest --test-dir build --output-on-failure
```

## 14. Scope

The project proposal in `../project-proposal.pptx` presents the overall course-project direction: model a DFG placement and scheduling problem for a Tensor Streaming Processor-like architecture, then solve it with a MILP-based method. The proposal emphasizes spatial-temporal scheduling, data-instruction rendezvous, deterministic routing, and cycle-accurate issue-time decisions.

This repository implements the concrete software prototype for that direction. It provides:

- a C++ DFG parser and validator;
- a fixed slice-based hardware abstraction;
- an OR-Tools MILP model for placement, issue time, dependency timing, ICU conflicts, and stream routing;
- JSON solution export;
- SVG schedule-placement-route visualization;
- example and final-evaluation DFG workloads.

The proposal also describes a broader two-phase algorithm:

```text
DFG -> graph preprocessing -> tightened ILP -> solver -> visualization
```

In the proposal, graph preprocessing includes topological sorting, ASAP/ALAP bound computation, transitive closure, variable-bound tightening, and incomparable-pair detection. The intended evaluation compares a pure ILP baseline with a graph-reduced ILP method using metrics such as makespan, constraint satisfaction, variable-bound tightness, constraint-reduction rate, and solve time.

The current repository focuses on the end-to-end ILP prototype and visualization path. It directly builds the MILP from the parsed DFG and the slice domains encoded in each `.dfg` file. It records solver status, makespan, variable count, constraint count, operation placement, operation issue time, and route assignment. These outputs support the final report and can be extended into the proposal's full comparison methodology by adding the graph-preprocessing phase and running side-by-side baseline experiments.

The evaluation scope is also narrower than the proposal. The proposal planned a comparative evaluation between `Pure ILP (baseline)` and `Graph + ILP (ours)`, with emphasis on preprocessing quality, constraint-reduction rate, solve-time speedup, scalability within a target time budget, and 100% post-solve constraint satisfaction. This repository instead provides a single implemented ILP method and evaluates it on representative DFG workloads under `evaluation/data/`. For each workload, the current evaluation saves the concrete solver result under `evaluation/solutions/` and the schedule-placement-route figure under `evaluation/figures/`. The recorded metrics are status, makespan, variable count, constraint count, operation assignments, and stream-route assignments. It does not yet report ASAP/ALAP bound tightness, transitive-closure pruning rate, baseline-versus-optimized speedup, or a separate constraint-reduction-rate table.

## 15. What This Repository Demonstrates

This repository demonstrates how algorithms from integrated circuit engineering and operations research can be combined to solve a concrete placement-and-scheduling problem:

- DFG parsing and validation;
- hardware-aware legal placement;
- exact and lower-bound timing constraints;
- resource conflict constraints;
- stream routing constraints;
- makespan minimization;
- solution export and visualization.

The result is a working C++ OR-Tools prototype that can solve small to medium DFG placement/scheduling instances and generate figures suitable for a course project report.
