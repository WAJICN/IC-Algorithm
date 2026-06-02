#include "model/objective.h"

#include "ortools/linear_solver/linear_solver.h"

namespace dfg_ilp {

void AddMakespanObjective(const Dfg& graph, VariableContext* vars) {
  for (int i = 0; i < graph.NumOps(); ++i) {
    operations_research::MPConstraint* constraint =
        vars->solver->MakeRowConstraint(graph.operation(i).dfunc,
                                        vars->solver->infinity(),
                                        VarName("makespan_ge_finish", i));
    constraint->SetCoefficient(vars->makespan, 1.0);
    constraint->SetCoefficient(vars->issue[i], -1.0);
  }

  operations_research::MPObjective* objective = vars->solver->MutableObjective();
  objective->SetCoefficient(vars->makespan, 1.0);
  objective->SetMinimization();
}

}  // namespace dfg_ilp
