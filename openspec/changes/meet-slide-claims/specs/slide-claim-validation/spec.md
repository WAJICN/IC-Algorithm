## ADDED Requirements

### Requirement: Domain-Level Solution Verification

The system SHALL verify feasible or optimal solver outputs with a domain-level checker independent of OR-Tools' internal solution verifier.

#### Scenario: Successful domain check
- **WHEN** a solver result satisfies placement, timing, non-overlap, stream, and makespan semantics
- **THEN** the domain checker SHALL report success with zero errors

#### Scenario: Failed domain check
- **WHEN** a solver result violates any domain-level constraint
- **THEN** the domain checker SHALL report failure and include at least one human-readable error

#### Scenario: Domain check is exported
- **WHEN** a solution JSON file is written
- **THEN** it SHALL include domain-check status and error count

### Requirement: Full-Duration Same-Slice Non-Overlap

The solver SHALL prevent operations assigned to the same physical slice from having overlapping execution intervals.

#### Scenario: Same-slice intervals cannot overlap
- **WHEN** two operations are assigned to the same slice
- **THEN** either the first operation SHALL finish no later than the second operation starts or the second operation SHALL finish no later than the first operation starts

#### Scenario: Different-slice intervals may overlap
- **WHEN** two operations are assigned to different slices
- **THEN** the same-slice non-overlap constraint SHALL NOT force an ordering between their execution intervals

#### Scenario: Baseline and graph modes use the same resource semantics
- **WHEN** either baseline mode or graph-preprocessed mode constructs resource constraints
- **THEN** both modes SHALL enforce full-duration same-slice non-overlap

### Requirement: Safe Resource-Pair Pruning

Graph-preprocessed mode SHALL only prune resource pairs when doing so preserves full-duration non-overlap correctness.

#### Scenario: Baseline resource pairs
- **WHEN** baseline mode constructs the ILP
- **THEN** it SHALL consider all operation pairs for full-duration non-overlap

#### Scenario: Graph resource pairs
- **WHEN** graph-preprocessed mode constructs the ILP
- **THEN** it SHALL include all incomparable operation pairs and any comparable operation pair not proven non-overlapping by dependency timing

### Requirement: Slide Claim Evaluation Evidence

The project SHALL provide repeatable evaluation outputs for existing workloads and scalability workloads.

#### Scenario: Existing workload comparison
- **WHEN** the comparison script is run on existing evaluation workloads
- **THEN** it SHALL record status, makespan, variable count, constraint count, solve time, pair reduction rate, and domain-check status for baseline and graph modes

#### Scenario: Scalability run
- **WHEN** the scalability evaluation is run
- **THEN** it SHALL solve generated graph-preprocessed workloads up to the configured target size and record status, solve time, makespan, variable count, constraint count, and domain-check status

#### Scenario: Slide claim summary
- **WHEN** evaluation completes
- **THEN** the generated summary SHALL contain enough data to support or qualify the slide claims about constraint satisfaction, constraint reduction, solve-time speedup, and scalability
