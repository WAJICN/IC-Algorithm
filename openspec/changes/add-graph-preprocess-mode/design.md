## Context

The repository already contains a direct OR-Tools ILP solver that parses a DFG, creates issue/placement/stream variables, adds timing/resource/stream constraints, minimizes makespan, and exports JSON. This is a suitable Pure ILP baseline.

The proposal slides describe a broader method:

```text
DFG -> graph preprocessing -> tightened ILP -> solver -> evaluation
```

That method requires graph-derived metadata before ILP construction. The key implementation constraint is comparability: baseline mode and graph-preprocessed mode should keep the same core objective and modeling semantics so differences in variable count, constraint count, solve time, and makespan are attributable to preprocessing.

## Goals / Non-Goals

**Goals:**

- Preserve the current direct ILP path as an explicit baseline.
- Add a graph-preprocessed mode that computes topological order, ASAP/ALAP bounds, transitive reachability, and incomparable pairs.
- Use graph preprocessing to tighten issue variable bounds and prune operation-pair resource constraints.
- Export enough metrics to support the final presentation: mode, solve time, pair reduction rate, average issue-window size, variable count, constraint count, status, and makespan.
- Keep the DFG input format unchanged.

**Non-Goals:**

- Do not change stream-route semantics in this change.
- Do not replace the current same-slice/same-issue ICU model with full-duration non-overlap in this change.
- Do not add an external benchmarking framework.
- Do not infer operation type legality from names; legal slices remain encoded by `slices=`.

## Decisions

### Decision: Add a Solver Mode Instead of Replacing the Existing Solver

Add a configuration value such as:

```cpp
enum class SolverMode {
  kBaseline,
  kGraphPreprocessed,
};
```

Baseline mode should reproduce the current behavior. Graph-preprocessed mode should reuse the same model builder with additional preprocessing inputs.

Alternative considered: always enable preprocessing. That would make it harder to produce a baseline comparison and could change existing saved outputs unexpectedly.

### Decision: Use Conservative Bounds Based on Minimum Feasible Slice Distance

For an edge `u -> v`, placement is not known before solving. The preprocessing phase should compute a safe lower-bound latency:

```text
min_dist(u,v) = min |p - q| for p in feasible_slices(u), q in feasible_slices(v)
min_latency(u,v) = dfunc(u) + KStream + min_dist(u,v)
```

Then:

```text
ASAP(v) = max over predecessors u of ASAP(u) + min_latency(u,v)
ALAP(u) = min over successors v of ALAP(v) - min_latency(u,v)
```

Sink ALAP can start from `hardware.max_issue_time`. These bounds are conservative and should not exclude feasible ILP solutions.

Alternative considered: derive bounds from a baseline solution makespan. That could be tighter but requires solving the baseline first, making graph mode slower and less clean as an independent solver path.

### Decision: Use Transitive Closure to Identify Incomparable Operation Pairs

The preprocessing phase should compute:

```text
reachable[u][v] = true if u can reach v through dependency edges
```

An incomparable pair is:

```text
P = {(u,v) | u < v and !reachable[u][v] and !reachable[v][u]}
```

Graph mode should create slice ICU pair variables and constraints only for `P`, because comparable pairs already have a dependency ordering in the DAG.

Alternative considered: keep all operation pairs but only report potential reductions. That would not produce the intended graph-reduced ILP.

### Decision: Keep Current Resource Semantics for Fair Baseline Comparison

Current resource constraints prevent two operations from using the same slice at the same issue cycle. This change should prune those existing pair constraints, not redefine them as full-duration non-overlap. Full-duration non-overlap is a separate modeling improvement and would invalidate direct comparisons with the baseline.

### Decision: Export Metrics in Solution JSON

Extend `SolveResult` with optional fields:

```text
mode
solve_time_ms
preprocess_time_ms
total_op_pairs
active_op_pairs
pruned_op_pairs
pair_reduction_rate
avg_issue_window
```

Baseline mode can report `total_op_pairs == active_op_pairs` and zero pair reduction. Graph mode reports the actual incomparable-pair reduction.

## Risks / Trade-offs

- Conservative ASAP/ALAP bounds may be modest, especially when feasible slice domains are wide. This still satisfies the graph-preprocessing claim, but solve-time speedup may come mostly from pair pruning.
- Pruning only operation-pair ICU constraints does not reduce pairwise stream constraints. Larger edge-heavy DFGs may still have significant quadratic stream-model cost.
- If comparable operations can still conflict under the current resource model because dependency timing allows equal issue cycles in corner cases, pruning could be unsafe. The implementation should test that timing constraints force strict enough separation or avoid pruning pairs when safety is ambiguous.
- `ALAP` initialized from `max_issue_time` may be too loose for some workloads. A future enhancement could compute a heuristic horizon or optionally use a known baseline makespan.

## Migration Plan

1. Add the new mode with baseline as the default.
2. Add preprocessing data structures and tests independent from the ILP solver.
3. Thread optional preprocessing results into variable and constraint creation.
4. Export metrics without removing existing JSON fields.
5. Re-run existing evaluation workloads in both modes and compare outputs.

Rollback is simple: keep baseline mode as the default and disable or ignore graph mode if preprocessing introduces regressions.

## Open Questions

- Should graph mode be the default once validated, or should the CLI default remain baseline for reproducibility?
- Should `ALAP` use `hardware.max_issue_time`, a user-provided horizon, or an optional baseline makespan?
- Should pair pruning apply only to ICU constraints first, or later also to stream constraints where graph ordering makes conflicts impossible?
