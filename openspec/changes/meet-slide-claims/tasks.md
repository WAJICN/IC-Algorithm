## 1. Domain Checker

- [x] 1.1 Add `model/solution_checker.h` and `model/solution_checker.cpp` with structured check results.
- [x] 1.2 Verify operation placement, issue/finish consistency, and makespan coverage.
- [x] 1.3 Verify DFG dependency timing semantics for exact compute-involved edges and lower-bound memory-memory edges.
- [x] 1.4 Verify full-duration same-slice non-overlap.
- [x] 1.5 Verify stream direction, stream ID range, route interval, phase, and pairwise stream conflict semantics.
- [x] 1.6 Integrate the domain checker into `IlpSolver::Solve` for feasible/optimal results.
- [x] 1.7 Export domain-check status and errors in JSON.
- [x] 1.8 Add tests that demonstrate both passing and failing domain checks.

## 2. Full-Duration Non-Overlap Model

- [x] 2.1 Add operation interval-order variables for active resource pairs.
- [x] 2.2 Replace same-slice/same-issue-cycle ICU constraints with full-duration same-slice non-overlap constraints.
- [x] 2.3 Update graph-mode active resource-pair selection to include any comparable pair not provably separated.
- [x] 2.4 Update metrics so pair reduction reflects the new active full-duration resource-pair set.
- [x] 2.5 Add solver tests that reject or avoid overlapping intervals on the same slice.
- [x] 2.6 Re-run unit tests and existing evaluation workloads under the stronger model.

## 3. Scalability Evidence

- [x] 3.1 Add a deterministic scalability workload generator for configurable operation counts up to 100.
- [x] 3.2 Add a scalability runner that solves generated workloads in graph-preprocessed mode with the slide time budget.
- [x] 3.3 Export scalability results to CSV with status, solve time, makespan, variables, constraints, and domain-check status.
- [x] 3.4 Add documentation for reproducing the scalability run.

## 4. Slide Claim Reporting

- [x] 4.1 Update the mode comparison script to include domain-check fields.
- [x] 4.2 Regenerate baseline-vs-graph comparison results for existing workloads.
- [x] 4.3 Generate scalability results up to the configured target size.
- [x] 4.4 Add a short markdown summary that states which slide claims are proven, partially supported, or not supported by the generated results.
