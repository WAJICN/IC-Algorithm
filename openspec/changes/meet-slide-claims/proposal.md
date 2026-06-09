## Why

The current repository now supports the slide's Graph + ILP preprocessing story, but three slide-facing claims still need stronger evidence or implementation: scalability evidence, domain-level post-solve constraint verification, and full-duration same-slice non-overlap. This change closes those gaps so final presentation claims can be backed by code and generated evaluation artifacts.

## What Changes

- Add a domain-level solution checker that verifies the solved schedule against the DFG and hardware semantics, independently of OR-Tools' internal `VerifySolution`.
- Replace the current same-slice/same-issue-cycle ICU conflict model with full-duration non-overlap for operations assigned to the same slice.
- Update baseline and graph-preprocessed modes to share the stronger full-duration non-overlap model so comparisons remain fair.
- Export domain-check results in solution JSON and comparison CSVs.
- Add scalability/evidence workloads or generation scripts to test and report graph-preprocessed solves up to the slide target size.
- Re-run evaluation and scalability comparisons to produce proof artifacts for the slide claims.

## Capabilities

### New Capabilities

- `slide-claim-validation`: Adds domain-level correctness verification, full-duration non-overlap semantics, and scalability evidence needed to substantiate the slide claims.

### Modified Capabilities

- None.

## Impact

- Affected code: `model/constraints/slice_icu.*`, `model/variables.*`, `model/ilp_solver.*`, `model/solution.*`, `visualization/exporter.*`, `tools/solve.cpp`, evaluation scripts, tests, and documentation.
- Solver behavior impact: both baseline and graph modes will enforce stricter resource constraints for same-slice operation intervals.
- Metrics impact: generated JSON/CSV outputs will include domain-check status and details.
- Evaluation impact: existing comparison outputs should be regenerated after the stronger model lands.
