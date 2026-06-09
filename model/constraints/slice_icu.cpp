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

std::vector<int> SharedSlices(const VariableContext& vars, int first, int second) {
  std::vector<int> slices;
  for (const auto& [slice, y] : vars.place[first]) {
    if (vars.place[second].find(slice) != vars.place[second].end()) {
      slices.push_back(slice);
    }
  }
  return slices;
}

}  // namespace

void AddSliceIcuConstraints(const Dfg& graph, const Hardware& hw,
                            const std::vector<OpPairKey>& distance_pairs,
                            const std::vector<OpPairKey>& resource_pairs,
                            VariableContext* vars) {
  const int n = graph.NumOps();
  for (const OpPairKey& key : distance_pairs) {
    AddAbsEquals(vars, vars->abs_pos_delta.at(key), vars->pos[key.first],
                 vars->pos[key.second], vars->pos_order.at(key), hw.big_m,
                 "abs_pos", key.first, key.second);
  }

  for (const OpPairKey& key : resource_pairs) {
    const int i = key.first;
    const int j = key.second;
    for (const int slice : SharedSlices(*vars, i, j)) {
      operations_research::MPConstraint* i_before_j =
          vars->solver->MakeRowConstraint(
              -vars->solver->infinity(), 3.0 * hw.big_m,
              VarName("non_overlap_i_before_j", i, j, slice));
      i_before_j->SetCoefficient(vars->issue[i], 1.0);
      i_before_j->SetCoefficient(vars->issue[j], -1.0);
      i_before_j->SetCoefficient(vars->place[i].at(slice), hw.big_m);
      i_before_j->SetCoefficient(vars->place[j].at(slice), hw.big_m);
      i_before_j->SetCoefficient(vars->op_interval_order.at(key), hw.big_m);
      i_before_j->SetBounds(-vars->solver->infinity(),
                            3.0 * hw.big_m - graph.operation(i).dfunc);

      operations_research::MPConstraint* j_before_i =
          vars->solver->MakeRowConstraint(
              -vars->solver->infinity(), 2.0 * hw.big_m,
              VarName("non_overlap_j_before_i", i, j, slice));
      j_before_i->SetCoefficient(vars->issue[j], 1.0);
      j_before_i->SetCoefficient(vars->issue[i], -1.0);
      j_before_i->SetCoefficient(vars->place[i].at(slice), hw.big_m);
      j_before_i->SetCoefficient(vars->place[j].at(slice), hw.big_m);
      j_before_i->SetCoefficient(vars->op_interval_order.at(key), -hw.big_m);
      j_before_i->SetBounds(-vars->solver->infinity(),
                            2.0 * hw.big_m - graph.operation(j).dfunc);
    }
  }
}

}  // namespace dfg_ilp
