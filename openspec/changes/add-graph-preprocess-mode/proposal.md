## Why

The project proposal claims a two-phase Graph + ILP method with topological preprocessing, ASAP/ALAP bound tightening, transitive-closure pruning, and baseline comparison. The current repository implements only the direct ILP path, which is useful as the Pure ILP baseline but does not yet support the claimed graph-preprocessed method or its evaluation metrics.

## What Changes

- Add an explicit solver mode switch so the existing direct ILP remains available as the baseline and a new graph-preprocessed ILP path can be selected.
- Add a graph preprocessing phase that computes topological order, conservative ASAP/ALAP issue bounds, transitive reachability, and incomparable operation pairs.
- Use preprocessing results in graph mode to tighten issue-time variable bounds and limit operation resource-conflict helper variables/constraints to incomparable pairs.
- Export preprocessing and evaluation metrics, including mode, solve time, pair reduction rate, and issue-window tightness.
- Add tests and evaluation commands that compare baseline ILP against graph-preprocessed ILP on existing DFG workloads.
- No breaking changes to the existing DFG syntax or baseline solver behavior.

## Capabilities

### New Capabilities

- `graph-preprocessed-ilp`: Supports a second solver path that applies graph preprocessing before ILP construction and reports comparison metrics against the baseline path.

### Modified Capabilities

- None.

## Impact

- Affected code: `model/ilp_solver.*`, `model/variables.*`, `model/constraints/slice_icu.*`, `model/solution.*`, `tools/solve.cpp`, `visualization/exporter.*`, and tests.
- New code: a preprocessing module, likely under `model/preprocess.*`.
- Output impact: solution JSON gains optional mode, timing, and preprocessing metric fields.
- CLI impact: `solve` gains a mode flag while preserving current default behavior.
- Evaluation impact: existing evaluation data can be run twice, once as baseline and once as graph-preprocessed, to produce the slide-claimed comparison table.
