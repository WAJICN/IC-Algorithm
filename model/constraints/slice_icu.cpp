#include "model/constraints/slice_icu.h"

#include "ortools/linear_solver/linear_solver.h"

namespace dfg_ilp {
namespace {

void AddAbsEquals(VariableContext* vars, MPVariable* abs_value, MPVariable* lhs,
                  MPVariable* rhs, MPVariable* order, int big_m,
                  const std::string& name_prefix, int i, int j) {
  // order = 1 selects lhs >= rhs; order = 0 selects rhs >= lhs.
  operations_research::MPConstraint* c1 = vars->solver->MakeRowConstraint(
      0.0, vars->solver->infinity(), VarName(name_prefix + "_ge_a", i, j));
  c1->SetCoefficient(abs_value, 1.0);
  c1->SetCoefficient(lhs, -1.0);
  c1->SetCoefficient(rhs, 1.0);

  operations_research::MPConstraint* c2 = vars->solver->MakeRowConstraint(
      0.0, vars->solver->infinity(), VarName(name_prefix + "_ge_b", i, j));
  c2->SetCoefficient(abs_value, 1.0);
  c2->SetCoefficient(lhs, 1.0);
  c2->SetCoefficient(rhs, -1.0);

  operations_research::MPConstraint* c3 = vars->solver->MakeRowConstraint(
      -vars->solver->infinity(), big_m, VarName(name_prefix + "_le_a", i, j));
  c3->SetCoefficient(abs_value, 1.0);
  c3->SetCoefficient(lhs, -1.0);
  c3->SetCoefficient(rhs, 1.0);
  c3->SetCoefficient(order, big_m);

  operations_research::MPConstraint* c4 = vars->solver->MakeRowConstraint(
      -vars->solver->infinity(), 0.0, VarName(name_prefix + "_le_b", i, j));
  c4->SetCoefficient(abs_value, 1.0);
  c4->SetCoefficient(lhs, 1.0);
  c4->SetCoefficient(rhs, -1.0);
  c4->SetCoefficient(order, -big_m);
}

}  // namespace

void AddSliceIcuConstraints(const Dfg& graph, const Hardware& hw,
                            VariableContext* vars) {
  const int n = graph.NumOps();
  for (int i = 0; i < n; ++i) {
    for (int j = i + 1; j < n; ++j) {
      const OpPairKey key{i, j};
      AddAbsEquals(vars, vars->abs_pos_delta.at(key), vars->pos[i], vars->pos[j],
                   vars->pos_order.at(key), hw.big_m, "abs_pos", i, j);
      AddAbsEquals(vars, vars->abs_issue_delta.at(key), vars->issue[i],
                   vars->issue[j], vars->issue_order.at(key), hw.big_m,
                   "abs_issue", i, j);

      operations_research::MPConstraint* no_conflict =
          vars->solver->MakeRowConstraint(1.0, vars->solver->infinity(),
                                          VarName("slice_cycle_no_conflict", i, j));
      no_conflict->SetCoefficient(vars->abs_pos_delta.at(key), 1.0);
      no_conflict->SetCoefficient(vars->abs_issue_delta.at(key), 1.0);
      no_conflict->SetCoefficient(vars->same_slice_same_cycle.at(key), hw.big_m);
    }
  }

  for (int i = 0; i < n; ++i) {
    operations_research::MPConstraint* cap = vars->solver->MakeRowConstraint(
        -vars->solver->infinity(), -1.0,
        VarName("icu_capacity", i));
    for (int j = 0; j < n; ++j) {
      if (i == j) {
        continue;
      }
      const OpPairKey key{std::min(i, j), std::max(i, j)};
      cap->SetCoefficient(vars->same_slice_same_cycle.at(key), 1.0);
    }
    for (const auto& [slice, y] : vars->place[i]) {
      cap->SetCoefficient(y, -hw.IcuCapacity(slice));
    }
  }
}

}  // namespace dfg_ilp
