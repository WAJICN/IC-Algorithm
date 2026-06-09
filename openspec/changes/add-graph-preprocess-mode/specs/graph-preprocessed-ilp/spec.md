## ADDED Requirements

### Requirement: Solver Mode Selection

The solver SHALL support both a baseline ILP mode and a graph-preprocessed ILP mode.

#### Scenario: Baseline mode preserves existing direct ILP behavior
- **WHEN** the solver is run in baseline mode
- **THEN** it SHALL construct the ILP without graph-derived issue-bound tightening or incomparable-pair pruning

#### Scenario: Graph mode applies preprocessing
- **WHEN** the solver is run in graph-preprocessed mode
- **THEN** it SHALL run graph preprocessing before ILP variable and constraint creation

### Requirement: Graph Preprocessing Output

The graph preprocessing phase SHALL compute topological order, conservative ASAP issue bounds, conservative ALAP issue bounds, transitive reachability, and incomparable operation pairs for a valid DFG.

#### Scenario: Preprocessing a DAG
- **WHEN** graph preprocessing receives a valid acyclic DFG
- **THEN** it SHALL return a topological order containing every operation exactly once
- **AND** it SHALL return one ASAP bound and one ALAP bound per operation
- **AND** it SHALL return reachability information for operation pairs
- **AND** it SHALL return the set of incomparable operation pairs

#### Scenario: Comparable pair exclusion
- **WHEN** operation `u` reaches operation `v` or `v` reaches `u` in the DFG
- **THEN** preprocessing SHALL NOT include `(u,v)` in the incomparable operation-pair set

#### Scenario: Incomparable pair inclusion
- **WHEN** neither operation in a pair can reach the other through DFG edges
- **THEN** preprocessing SHALL include that pair in the incomparable operation-pair set

### Requirement: Conservative Issue Bound Tightening

Graph-preprocessed mode SHALL create issue variables using graph-derived bounds that do not exclude feasible schedules.

#### Scenario: ASAP lower bound
- **WHEN** graph-preprocessed mode creates an issue variable for operation `v`
- **THEN** the variable lower bound SHALL be at least the computed ASAP bound for `v`

#### Scenario: ALAP upper bound
- **WHEN** graph-preprocessed mode creates an issue variable for operation `v`
- **THEN** the variable upper bound SHALL be at most the computed ALAP bound for `v`

#### Scenario: Bound validity
- **WHEN** preprocessing computes bounds for an operation
- **THEN** the ASAP bound SHALL be less than or equal to the ALAP bound

### Requirement: Incomparable-Pair Constraint Pruning

Graph-preprocessed mode SHALL use the incomparable operation-pair set to reduce operation resource-conflict helper variables and constraints.

#### Scenario: Baseline creates all operation pairs
- **WHEN** the solver is run in baseline mode for a graph with `N` operations
- **THEN** the active operation-pair count SHALL equal `N * (N - 1) / 2`

#### Scenario: Graph mode prunes comparable operation pairs
- **WHEN** the solver is run in graph-preprocessed mode
- **THEN** operation-pair resource-conflict variables and constraints SHALL be created only for incomparable operation pairs

#### Scenario: Reduction metrics are computed
- **WHEN** graph-preprocessed mode completes model construction
- **THEN** the result SHALL report total operation pairs, active operation pairs, pruned operation pairs, and pair reduction rate

### Requirement: Solution Metrics Export

The solver SHALL export mode and preprocessing metrics in the solution result without removing existing solution fields.

#### Scenario: Baseline JSON output
- **WHEN** baseline mode writes a solution JSON file
- **THEN** the JSON SHALL include the existing status, objective value, makespan, variable count, constraint count, operation assignments, and edge assignments
- **AND** it SHALL include a mode value identifying baseline mode

#### Scenario: Graph mode JSON output
- **WHEN** graph-preprocessed mode writes a solution JSON file
- **THEN** the JSON SHALL include the existing solution fields
- **AND** it SHALL include preprocessing metrics for issue bounds and operation-pair reduction

#### Scenario: Solve timing is reported
- **WHEN** either solver mode completes
- **THEN** the result SHALL report solver wall-clock time in milliseconds

### Requirement: Baseline and Graph Mode Comparison

The project SHALL provide tests or evaluation commands that allow the baseline and graph-preprocessed modes to be compared on the same DFG inputs.

#### Scenario: Comparable result fields
- **WHEN** the same DFG is solved in both modes
- **THEN** both outputs SHALL report status, makespan, variable count, constraint count, and solve time

#### Scenario: Graph mode does not weaken correctness checks
- **WHEN** graph-preprocessed mode finds an optimal or feasible solution
- **THEN** the solution SHALL pass the same OR-Tools verification used by baseline mode
