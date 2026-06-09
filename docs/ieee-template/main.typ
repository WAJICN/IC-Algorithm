#import "@preview/charged-ieee:0.1.4": ieee

#let opbox(body, fill: rgb("#eeeeee")) = box(
  width: 100%,
  inset: 5pt,
  stroke: 0.6pt + rgb("#555555"),
  fill: fill,
  radius: 1.5pt,
  align(center, text(size: 8pt, body)),
)

#let arrow = align(center, text(size: 8pt)[->])

#let system_figure = figure(
  placement: top,
  grid(
    columns: (1fr, 0.14fr, 1fr, 0.14fr, 1fr),
    rows: (auto, auto, auto),
    gutter: 3pt,
    align: horizon,
    opbox([DFG\ parser], fill: rgb("#f4f4f4")), arrow,
    opbox([Graph\ analysis], fill: rgb("#e6e6e6")), arrow,
    opbox([ILP\ model], fill: rgb("#d8d8d8")),
    opbox([legal slices\ dependencies], fill: rgb("#f4f4f4")), [],
    opbox([ASAP/ALAP\ pair pruning], fill: rgb("#e6e6e6")), [],
    opbox([placement\ timing\ streams], fill: rgb("#d8d8d8")),
    [], [], opbox([Checker\ JSON/CSV], fill: rgb("#cccccc")), [], [],
  ),
  caption: [Solver flow. Graph preprocessing narrows issue windows and removes safe operation-pair constraints before the shared ILP model is solved and checked.]
)

#show: ieee.with(
  title: [Graph-Preprocessed ILP Scheduling for DFG Placement],
  abstract: [
    We study placement and scheduling of data-flow graphs on a linear slice-based architecture with streaming communication. The implementation provides a direct integer-linear-programming baseline and a graph-preprocessed mode that tightens issue-time windows and prunes resource-pair constraints before constructing the same domain model. A post-solve checker independently validates placement, dependency timing, full-duration same-slice non-overlap, stream routing, and makespan semantics. On solved existing workloads, graph preprocessing preserves makespan and reduces constraints by 5.7%--21.8%. Generated workloads up to 100 operations produce feasible or optimal schedules within a 60 s budget, with every solved result passing the domain checker.
  ],
  authors: (
    (
      name: "Junchi Wu",
      department: [Student ID: 2501111908],
      organization: [Peking University],
      location: [Beijing, China],
    ),
    (
      name: "Feiyang Wu",
      department: [Student ID: 2501111907],
      organization: [Peking University],
      location: [Beijing, China],
    ),
  ),
  index-terms: ("integer linear programming", "DFG scheduling", "graph preprocessing", "placement", "verification"),
  bibliography: none,
  figure-supplement: [Fig.],
)

= Motivation and Background

Specialized data-flow accelerators expose high throughput only when computation, memory movement, and inter-slice streams are scheduled together. In this project, each DFG operation is assigned to a hardware slice and issue cycle, while each edge is routed through a directional stream. A direct ILP formulation is attractive because it states the architecture constraints exactly, but it scales poorly as every pair of operations and every pair of streams can introduce binary decisions.

The target abstraction is deliberately small but captures the essential coupling in the placement problem. Compute operations often have narrow legal slice sets, memory operations may live on banks away from the vector slice, and edge latency depends on physical distance. A placement decision therefore changes both local resource use and communication timing. Treating placement first and scheduling later can miss feasible schedules or accept schedules that violate the streaming model; treating all decisions together gives correctness but increases the size of the mixed-integer search.

The project therefore separates two concerns. The baseline keeps the full ILP model and serves as the reference. The graph-preprocessed mode first analyzes the DFG topology to compute issue-time bounds and identify operation pairs whose relative order is already guaranteed by dependency timing. Both modes then use the same placement, timing, stream, and full-duration non-overlap semantics. This keeps the comparison fair: preprocessing may reduce model size, but it does not relax correctness.

#system_figure

= Problem Formulation

Given a DFG $G=(V,E)$, each operation $i in V$ has a kind, a duration $d_i$, and an optional feasible slice set $S_i$. The solver chooses integer issue time $t_i$, slice position $p_i$, and finish time $f_i=t_i+d_i$. For each edge $(i,j)$, it also chooses a stream id, direction, route interval, and phase.

The hardware is modeled as a one-dimensional slice line from $p_min$ to $p_max$. Binary placement variables $y_(i,p)$ select exactly one legal slice for each operation. The position variable is then linked to the selected slice by $p_i=sum_p p y_(i,p)$. This representation keeps operation legality local while still exposing positions to timing and routing constraints.

The objective is to minimize makespan:

$ min M quad "s.t." quad M >= t_i + d_i, forall i in V. $

For compute-involved edges, the implementation enforces exact timing with stream latency $k$ and physical slice distance:

$ t_j - t_i = d_i + k + |p_i - p_j|. $

For memory-to-memory edges, the same expression is a lower bound, allowing later issue times when memory movement is not tied to compute-cycle equality. The full-duration resource constraint forbids same-slice interval overlap:

$ p_i = p_j => (t_i + d_i <= t_j) or (t_j + d_j <= t_i). $

The implementation encodes this disjunction with one binary order variable per active operation pair and per-slice guards from the placement selectors. This is stronger than forbidding only equal issue cycles: two operations of duration greater than one may issue at different cycles and still overlap. The stricter form is necessary for a capacity-one slice model.

Stream constraints select one direction and one stream id for every edge, bind the route interval to source/destination positions, and require non-conflicting phase order for edges sharing stream id, direction, and route overlap. These constraints are exported to JSON, then checked independently by a domain verifier.

= Algorithms

== Baseline ILP

The baseline constructs the model over all operations and all operation pairs. It creates placement selectors, issue variables, absolute position-distance variables for timing, binary interval-order variables for same-slice non-overlap, and stream-pair variables for route conflicts. This mode is the correctness reference and uses the same objective and domain checker as the graph mode.

The baseline is intentionally not optimized by graph reachability. It is useful because every pair of operations is considered for resource conflicts, so it exposes the full cost of the direct ILP. It also gives a clean comparison point for preprocessing: if graph mode changes makespan or violates the domain checker, the change is a correctness bug rather than a modeling difference.

== Graph Preprocessing

Graph preprocessing performs a topological traversal to compute ASAP and ALAP issue-time bounds. For each edge, the minimum possible latency is $d_i+k+min |p_i-p_j|$ over legal slices. Forward propagation gives earliest times; reverse propagation gives latest times under the global issue horizon. The same reachability analysis classifies operation pairs as comparable or incomparable.

The bounds are conservative because they use minimum feasible physical distance rather than a selected placement. They cannot remove a valid solution; they only exclude issue times that are impossible even under the best slice choices. In practice this reduces issue domains for downstream operations and can also make the branch-and-bound search easier because fewer integer values remain available.

Comparable pairs are pruned from full-duration resource constraints only when dependency timing proves they cannot overlap. Memory-memory direct edges are retained conservatively because they use lower-bound rather than exact timing. The resulting active pair set is:

$ P_"graph" = P_"incomparable" union P_"unsafe comparable". $

This policy is more conservative than simply dropping all reachable pairs. Compute-involved dependencies enforce exact arrival, so the predecessor's finish and communication latency determine the successor's issue. Memory-memory edges, however, allow slack and therefore may not prove interval separation on their own. Keeping these unsafe comparable pairs avoids turning preprocessing into an implicit relaxation.

== Domain-Level Checker

After an optimal or feasible solve, the checker reconstructs the schedule outside OR-Tools and verifies all domain semantics: legal slices, one assignment per operation, finish-time consistency, makespan coverage, exact or lower-bound edge timing, full-duration same-slice non-overlap, stream id bounds, stream direction, route intervals, phases, arrival timing, and pairwise stream conflicts. A feasible schedule is accepted only when this checker reports zero errors.

This checker is separate from the solver's numerical constraint verifier. The numerical verifier can confirm that linear rows are satisfied, but it does not explain whether the rows jointly match the intended accelerator semantics. The domain checker reports human-readable errors, making it useful for regression tests and for evaluating generated JSON results after the solver has exited.

= Experimental Results

Experiments use the regenerated CSV/JSON artifacts in `evaluation/mode_comparison` and `evaluation/scalability`. The time limit is 90 s for existing baseline-vs-graph comparisons and 60 s for generated scalability workloads. Rows marked optimal or feasible all pass the domain checker.

#figure(
  placement: top,
  image("figures/mode_comparison.pdf", width: 100%),
  caption: [Existing workload comparison. Graph preprocessing preserves makespan on solved workloads and reduces model constraints; solve-time speedup is workload-dependent.]
) <fig:mode>

#figure(
  placement: top,
  image("figures/scalability.pdf", width: 100%),
  caption: [Generated scalability workloads. The 75- and 100-operation cases reach feasible schedules at the 60 s limit and pass the domain checker.]
) <fig:scale>

#figure(
  placement: top,
  table(
    columns: (auto, auto, auto, auto, auto),
    align: (left, center, center, right, center),
    inset: (x: 4pt, y: 2.8pt),
    stroke: (x, y) => if y <= 1 { (top: 0.45pt) },
    fill: (x, y) => if y > 0 and calc.rem(y, 2) == 0 { rgb("#eeeeee") },
    table.header[Workload][Base][Graph][Cons. Red.][Check],
    [Vector], [opt], [opt], [15.6%], [pass],
    [CNN], [opt], [opt], [5.7%], [pass],
    [Residual], [opt], [opt], [5.7%], [pass],
    [Tree reduce], [inf], [inf], [--], [--],
    [Attention], [opt], [opt], [6.1%], [pass],
    [Memory mix], [opt], [opt], [21.8%], [pass],
  ),
  caption: [Existing workloads under full-duration non-overlap. `Tree reduce` is infeasible in both modes, revealing a legacy workload invalidated by stricter resource semantics.]
) <tab:existing>

@fig:mode and @tab:existing show that the graph mode reduces constraints on all solved existing workloads while preserving makespan. The largest measured solve-time speedup is 3.50x on the vector pipeline, but CNN, residual, and memory-mix workloads are not faster under the stronger model. Thus the supported performance claim is model-size reduction with workload-dependent speed, not universal acceleration.

The infeasible tree-reduction result is also informative. Under the earlier same-issue-cycle conflict model, the workload could place long operations on the same slice with overlapping execution intervals as long as their issue cycles differed. Full-duration non-overlap invalidates that schedule in both baseline and graph modes. We therefore treat this workload as a legacy negative test rather than as performance evidence.

The scalability study in @fig:scale reaches 100 operations. The 25- and 50-operation cases solve optimally; the 75- and 100-operation cases return feasible schedules at the 60 s budget. Pair reduction remains substantial but decreases for larger generated workloads because the generator intentionally combines a 50-operation dependent pipeline with independent fixed-slice operations to test full-duration non-overlap without making stream-pair constraints dominate the benchmark.

#figure(
  placement: none,
  table(
    columns: (auto, auto, auto, auto, auto, auto),
    align: (left, right, center, right, right, center),
    inset: (x: 4pt, y: 2.8pt),
    stroke: (x, y) => if y <= 1 { (top: 0.45pt) },
    fill: (x, y) => if y > 0 and calc.rem(y, 2) == 0 { rgb("#eeeeee") },
    table.header[Case][Ops][Status][Makespan][Time][Check],
    [Scale-25], [25], [opt], [75], [1.37 s], [pass],
    [Scale-50], [50], [opt], [149], [11.58 s], [pass],
    [Scale-75], [75], [feas.], [189], [60.18 s], [pass],
    [Scale-100], [100], [feas.], [239], [60.18 s], [pass],
  ),
  caption: [Scalability evidence from graph-preprocessed mode. The 75- and 100-operation workloads return feasible checked schedules at the configured time budget.]
) <tab:scale>

@tab:scale gives the exact status and timing for the generated workloads. The feasible rows are still useful because the objective is makespan minimization and the solver may stop at the time limit before proving optimality. The domain checker accepts both optimal and feasible rows only after validating the recovered schedule.

The scalability data should be read as a functional and modeling result rather than a proof of asymptotic efficiency. The current stream formulation still creates pairwise edge-conflict variables, so dense DFGs can become difficult even when operation-pair pruning is effective. The generated cases show that the implemented graph mode can produce checked schedules at the slide target size, while leaving room for future stream-conflict preprocessing.

= Conclusion

The repository now functionally supports the claimed baseline-plus-graph-preprocessing workflow. It has a direct ILP baseline, a graph-preprocessed mode, a stronger full-duration non-overlap model, and an independent domain checker that validates feasible schedules. The experimental artifacts support correctness, model-size reduction, and feasibility through 100 operations. The only claim that must be qualified is solve-time speedup: preprocessing reduces constraints and can accelerate selected workloads, but the observed speedup depends on workload structure and solver behavior.

Future work should target two bottlenecks. First, the stream conflict model can be preprocessed analogously to operation pairs by proving disjoint route intervals or incompatible directions before variables are created. Second, benchmark generation should include a wider range of feasible reduction and branching patterns so that the evaluation distinguishes graph-structure effects from stream-density effects.
