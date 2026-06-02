#include "model/constraints/legal_slice.h"

#include "ortools/linear_solver/linear_solver.h"

namespace dfg_ilp {

void AddLegalSliceConstraints(const Dfg& graph, const Hardware& hw,
                              VariableContext* vars) {
  for (int i = 0; i < graph.NumOps(); ++i) {
    operations_research::MPConstraint* one_slice =
        vars->solver->MakeRowConstraint(1.0, 1.0, VarName("one_slice", i));
    operations_research::MPConstraint* pos_def =
        vars->solver->MakeRowConstraint(0.0, 0.0, VarName("pos_def", i));
    pos_def->SetCoefficient(vars->pos[i], 1.0);

    for (const auto& [slice, y] : vars->place[i]) {
      one_slice->SetCoefficient(y, 1.0);
      pos_def->SetCoefficient(y, -static_cast<double>(slice));
    }
  }
}

}  // namespace dfg_ilp
