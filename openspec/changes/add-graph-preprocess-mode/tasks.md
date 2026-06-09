## 1. Preprocessing Module

- [x] 1.1 Add `model/preprocess.h` and `model/preprocess.cpp` with a `GraphPreprocessResult` data structure.
- [x] 1.2 Implement topological ordering for valid DFGs.
- [x] 1.3 Implement feasible-slice minimum distance for each DFG edge.
- [x] 1.4 Implement conservative ASAP issue-bound computation.
- [x] 1.5 Implement conservative ALAP issue-bound computation.
- [x] 1.6 Implement transitive reachability and incomparable operation-pair detection.
- [x] 1.7 Add unit tests for topo order, ASAP/ALAP bounds, reachability, and incomparable pairs.

## 2. Solver Mode Integration

- [x] 2.1 Add a solver mode enum/config field for baseline and graph-preprocessed modes.
- [x] 2.2 Add a CLI flag to select solver mode while preserving current default behavior.
- [x] 2.3 Thread optional preprocessing results through `IlpSolver::Solve`.
- [x] 2.4 Make core issue variable creation use preprocessing bounds in graph-preprocessed mode.
- [x] 2.5 Make slice ICU variable creation accept an explicit operation-pair set.
- [x] 2.6 Make slice ICU constraints iterate only over active operation pairs in graph-preprocessed mode.

## 3. Metrics and Export

- [x] 3.1 Extend `SolveResult` with solver mode, solve time, preprocess time, and preprocessing metrics.
- [x] 3.2 Populate baseline metrics with total operation-pair count and zero reduction.
- [x] 3.3 Populate graph-preprocessed metrics from `GraphPreprocessResult`.
- [x] 3.4 Extend JSON export with the new fields without removing existing fields.
- [x] 3.5 Print mode, solve time, and reduction summary from the CLI.

## 4. Verification and Evaluation

- [x] 4.1 Add a small solve test that runs both modes on the same DFG.
- [x] 4.2 Assert graph mode has active operation-pair count less than or equal to baseline for a graph with comparable pairs.
- [x] 4.3 Assert graph mode preserves feasible or optimal status on existing small solve coverage.
- [x] 4.4 Add documentation for running baseline versus graph-preprocessed mode.
- [x] 4.5 Re-run evaluation workloads in both modes and record status, makespan, variable count, constraint count, solve time, and pair reduction rate.
