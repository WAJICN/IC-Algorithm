# DFG Input Format

The parser uses a small line-oriented text format. Blank lines and `#`
comments are ignored.

## Operations

```text
op <id> <kind> <dfunc> [slices=<slice-spec>]
```

- `<id>`: unique operation name.
- `<kind>`: `comp`, `compute`, `mem`, or `memory`.
- `<dfunc>`: non-negative integer operation latency.
- `slices=`: optional feasible slice set. If omitted, all hardware-legal
  slices are allowed.

Slice specs are comma-separated integers or ranges:

```text
slices=-46..46
slices=-10..10,20,25..30
```

## Edges

```text
edge <src-id> <dst-id>
```

Edges are directed producer-to-consumer dependencies.

If either endpoint is a compute operation, the solver applies the exact timing
constraint:

```text
issue(src) + dfunc(src) + distance(src,dst) + KStream = issue(dst)
```

If both endpoints are memory operations, the solver applies the lower-bound
constraint:

```text
issue(src) + dfunc(src) + distance(src,dst) + KStream <= issue(dst)
```
