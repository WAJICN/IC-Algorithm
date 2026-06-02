#include "model/constraints/timing.h"

#include "ortools/linear_solver/linear_solver.h"

namespace dfg_ilp {
namespace {

MPVariable* PosDeltaForEdge(const Edge& edge, VariableContext* vars) {
  if (edge.src < edge.dst) {
    return vars->abs_pos_delta.at(OpPairKey{edge.src, edge.dst});
  }
  return vars->abs_pos_delta.at(OpPairKey{edge.dst, edge.src});
}

}  // namespace

void AddTimingConstraints(const Dfg& graph, const Hardware& hw,
                          VariableContext* vars) {
  for (int edge_index = 0; edge_index < graph.NumEdges(); ++edge_index) {
    const Edge& edge = graph.edge(edge_index);
    const Operation& src = graph.operation(edge.src);
    const Operation& dst = graph.operation(edge.dst);
    const bool exact = src.kind == OpKind::kCompute || dst.kind == OpKind::kCompute;
    const double rhs = -(src.dfunc + hw.k_stream);

    operations_research::MPConstraint* constraint = nullptr;
    if (exact) {
      constraint =
          vars->solver->MakeRowConstraint(rhs, rhs, VarName("timing_eq", edge_index));
    } else {
      constraint = vars->solver->MakeRowConstraint(
          -vars->solver->infinity(), rhs, VarName("timing_lb", edge_index));
    }

    constraint->SetCoefficient(vars->issue[edge.src], 1.0);
    constraint->SetCoefficient(vars->issue[edge.dst], -1.0);
    constraint->SetCoefficient(PosDeltaForEdge(edge, vars), 1.0);
  }
}

}  // namespace dfg_ilp
