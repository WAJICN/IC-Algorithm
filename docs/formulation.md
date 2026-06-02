# Place and Schedule ILP Formulation

This implementation follows the formulation in
`../zhen/or-test/formulation.md`.

Implemented decision variables:

- `issue(i)`: integer issue cycle.
- `pos(i)`: integer physical slice.
- `y(i,p)`: binary placement selector.
- edge stream variables: direction, stream ID, route interval, route phase.
- pairwise helper variables for absolute distances, ICU conflicts, and stream
  conflicts.

The objective minimizes a `makespan` variable constrained by:

```text
makespan >= issue(i) + dfunc(i)
```

This is the standard linear form of minimizing the final completion time of the
whole graph.
