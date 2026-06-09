# Slide Claim Summary

Generated on 2026-06-09 from:

- `evaluation/mode_comparison/summary.csv`
- `evaluation/scalability/summary.csv`

## Proven By Current Artifacts

- Domain-level correctness checking is implemented and exported. Every feasible or optimal row in the regenerated existing-workload comparison passes the checker with zero errors.
- Full-duration same-slice non-overlap is enforced in both baseline and graph-preprocessed modes. The previous `04_binary_tree_reduce` workload is now infeasible in both modes, which exposes schedule overlap that the old same-issue-cycle model allowed.
- Graph preprocessing reduces operation-pair and constraint pressure on solved existing workloads while preserving makespan. For solved existing workloads, graph mode keeps the same makespan as baseline and reduces constraints by 5.7% to 21.8%.
- The scalability runner reaches the slide target size. Generated graph-preprocessed workloads at 25, 50, 75, and 100 operations all produce feasible or optimal domain-checked schedules within the 60s per-workload budget.

## Partially Supported

- Solve-time speedup is workload-dependent, not universal. Existing solved workloads show one clear speedup (`01_vector_pipeline`, 3.50x), one modest speedup (`05_transform_attention`, 1.13x), and three regressions under the stronger full-duration model.
- Constraint reduction is proven, but not always large. Existing solved workloads reduce constraints by 5.7% to 21.8%; generated 75/100-op workloads reduce operation-pair constraints by 44.1% and 24.7% respectively because half of each workload is intentionally independent.

## Not Supported As A Blanket Claim

- A blanket "2x faster" claim is not supported by the regenerated measurements. The defensible wording is that graph preprocessing can reduce model size and may improve solve time, with speedup depending on workload structure.
- Existing workload `04_binary_tree_reduce` is not a valid solved benchmark under full-duration non-overlap. It should be described as an invalidated legacy workload or replaced with a non-overlapping reduction benchmark before using it as positive performance evidence.

## Key Measurements

| Workload | Baseline Status | Graph Status | Makespan | Graph Constraint Reduction | Graph Solve Speedup | Domain Check |
| --- | --- | --- | --- | --- | --- | --- |
| `01_vector_pipeline` | optimal | optimal | 14 | 15.6% | 3.50x | pass |
| `02_cnn_pipeline` | optimal | optimal | 320 | 5.7% | 0.77x | pass |
| `03_residual_block` | optimal | optimal | 164 | 5.7% | 0.92x | pass |
| `04_binary_tree_reduce` | infeasible | infeasible | n/a | n/a | n/a | n/a |
| `05_transform_attention` | optimal | optimal | 184 | 6.1% | 1.13x | pass |
| `06_memory_stream_mix` | optimal | optimal | 111 | 21.8% | 0.94x | pass |

| Scalability Workload | Ops | Status | Makespan | Solve Time Ms | Pair Reduction | Domain Check |
| --- | ---: | --- | ---: | ---: | ---: | --- |
| `scale_025` | 25 | optimal | 75 | 1369 | 100.0% | pass |
| `scale_050` | 50 | optimal | 149 | 11582 | 100.0% | pass |
| `scale_075` | 75 | feasible | 189 | 60183 | 44.1% | pass |
| `scale_100` | 100 | feasible | 239 | 60177 | 24.7% | pass |
