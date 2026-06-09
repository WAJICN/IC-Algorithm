## Context

The completed `add-graph-preprocess-mode` change made the direct ILP path an explicit baseline and added graph-preprocessed mode with ASAP/ALAP bounds and incomparable-pair pruning. It produced evaluation evidence that graph mode preserves makespan and reduces variables/constraints on the existing workloads.

The remaining slide gaps are different in kind:

```text
                    CURRENT STATE
┌─────────────────────────────────────────────────────┐
│ OR-Tools VerifySolution                             │
│   checks linear constraints numerically             │
├─────────────────────────────────────────────────────┤
│ Current ICU resource constraint                     │
│   same slice + same issue cycle is forbidden        │
├─────────────────────────────────────────────────────┤
│ Current evaluation                                  │
│   6 workloads, largest = 24 ops                     │
└─────────────────────────────────────────────────────┘

                    SLIDE-ALIGNED STATE
┌─────────────────────────────────────────────────────┐
│ Domain verifier                                     │
│   checks dependency, placement, non-overlap, stream │
├─────────────────────────────────────────────────────┤
│ Full-duration non-overlap                           │
│   same slice intervals may not overlap              │
├─────────────────────────────────────────────────────┤
│ Scalability evidence                                │
│   generated or curated workloads approaching 100 ops│
└─────────────────────────────────────────────────────┘
```

## Goals / Non-Goals

**Goals:**

- Implement an independent domain-level post-solve checker.
- Enforce full-duration same-slice non-overlap in both baseline and graph modes.
- Preserve baseline-vs-graph comparability under the stronger model.
- Export verification status and failure details.
- Add repeatable scalability evidence for workloads up to the slide target.

**Non-Goals:**

- Do not change the DFG input format unless a scalability generator needs optional parameters outside `.dfg` files.
- Do not change stream-route modeling unless the domain checker reveals a mismatch that must be fixed to verify current outputs.
- Do not claim universal 2x speedup unless measured results support it.

## Decisions

### Decision: Add a Domain Checker Module

Add a module such as:

```text
model/solution_checker.h
model/solution_checker.cpp
```

It should verify solved `SolveResult` objects against:

- operation placement within feasible and hardware-legal slices;
- operation finish equals `issue + dfunc`;
- makespan covers all operation finishes;
- DFG dependency timing:
  - compute-involved edges: exact equality;
  - memory-memory edges: lower-bound relation;
- full-duration same-slice non-overlap;
- stream assignment bounds, route interval consistency, direction consistency, and phase consistency;
- stream conflict semantics currently modeled by the ILP.

The checker should return structured results:

```cpp
struct DomainCheckResult {
  bool ok;
  std::vector<std::string> errors;
};
```

### Decision: Enforce Full-Duration Same-Slice Non-Overlap with Pairwise Ordering

For every active resource pair `(i,j)`, add a binary ordering variable:

```text
op_before(i,j) = 1 means i finishes before j starts
op_before(i,j) = 0 means j finishes before i starts
```

Guard both orders by same-slice detection or existing placement selectors. A straightforward formulation can use per-slice Big-M disjunctions:

```text
issue(i) + dfunc(i) <= issue(j) + M * (3 - y(i,p) - y(j,p) - op_before(i,j))
issue(j) + dfunc(j) <= issue(i) + M * (2 - y(i,p) - y(j,p) + op_before(i,j))
```

for each legal slice `p` shared by the two operations. This matches the proposal slides' non-overlap form more directly than the current absolute-issue helper.

Alternative considered: keep absolute issue deltas and require `abs_issue_delta >= min(dfunc(i), dfunc(j))`. That is insufficient because non-overlap requires direction-sensitive interval ordering, not just separation by the shorter duration.

### Decision: Keep Pair Pruning but Re-evaluate Safety

Graph mode currently prunes comparable operation pairs from resource constraints. With full-duration non-overlap, comparable pairs are usually already ordered by dependency timing, but safety should be verified. If any comparable pair can still overlap because memory-memory lower-bound timing or zero-latency operations permit overlap, graph mode should conservatively include that pair.

The safe active resource pair policy should be:

```text
baseline: all operation pairs
graph: incomparable pairs plus any comparable pair not provably separated by dependency timing
```

### Decision: Export Domain Check Results

Extend solution JSON with:

```json
"domain_check": {
  "ok": true,
  "error_count": 0,
  "errors": []
}
```

Comparison CSVs should include at least `domain_check_ok` and `domain_check_errors`.

### Decision: Add Scalability Evidence as Generated Workloads

Add a deterministic generator script for DAG workloads of configurable size, for example:

```bash
python3 evaluation/scripts/generate_scalability_workloads.py --sizes 25,50,75,100
```

Generated workloads should avoid hand-maintenance and produce DFGs that remain meaningful for placement/scheduling:

- chain/pipeline pattern;
- branching reduction pattern;
- mixed memory/compute pattern.

Then add a runner that solves these workloads in graph mode with the slide time budget and records status, solve time, makespan, variables, constraints, and domain-check status.

## Risks / Trade-offs

- Full-duration non-overlap is a stronger model and can increase solve time, especially for baseline mode.
- Some previous optimal makespans may change because the old model allowed same-slice interval overlap as long as issue cycles differed.
- Graph-mode pair pruning must be revisited carefully; unsafe pruning would undermine correctness.
- The 100-op target may require generated workloads with controlled structure. Arbitrary 100-op dense DAGs may be too hard for the current stream model.

## Migration Plan

1. Add the domain checker and test it on existing solved cases.
2. Replace same-issue ICU constraints with full-duration non-overlap constraints.
3. Run baseline and graph modes on existing evaluation workloads and update comparison outputs.
4. Add generated scalability workloads and graph-mode scalability summary.
5. Update docs and final slide wording based on measured results.

## Open Questions

- Should full-duration non-overlap be always enabled, or should an opt-in legacy flag be kept for comparing against old results?
- What generated workload shapes best represent the slide's `|V| <= 100` claim without overfitting?
- Should domain-check failure be fatal in `solve`, or exported as a result field with a nonzero CLI exit?
