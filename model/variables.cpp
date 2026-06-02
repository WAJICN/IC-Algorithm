#include "model/variables.h"

#include <algorithm>
#include <sstream>
#include <stdexcept>

namespace dfg_ilp {
namespace {

std::vector<int> LegalSlicesForOp(const Dfg& graph, const Hardware& hw,
                                  int op_index) {
  const Operation& op = graph.operation(op_index);
  if (!op.feasible_slices) {
    return hw.AllSlices();
  }
  return hw.IntersectLegalSlices(*op.feasible_slices);
}

}  // namespace

std::vector<int> VariableContext::FeasibleSlicesForOp(const Dfg& graph,
                                                      const Hardware& hw,
                                                      int op_index) const {
  return LegalSlicesForOp(graph, hw, op_index);
}

std::string VarName(const std::string& prefix, int a) {
  return prefix + "_" + std::to_string(a);
}

std::string VarName(const std::string& prefix, int a, int b) {
  return prefix + "_" + std::to_string(a) + "_" + std::to_string(b);
}

std::string VarName(const std::string& prefix, int a, int b, int c) {
  return prefix + "_" + std::to_string(a) + "_" + std::to_string(b) + "_" +
         std::to_string(c);
}

void CreateCoreVariables(const Dfg& graph, const Hardware& hw,
                         VariableContext* vars) {
  MPSolver* solver = vars->solver;
  const double infinity = solver->infinity();
  const int n = graph.NumOps();

  vars->issue.resize(n);
  vars->pos.resize(n);
  vars->place.resize(n);

  for (int i = 0; i < n; ++i) {
    vars->issue[i] =
        solver->MakeIntVar(0.0, hw.max_issue_time, VarName("issue", i));
    vars->pos[i] =
        solver->MakeIntVar(hw.min_slice, hw.max_slice, VarName("pos", i));

    const std::vector<int> feasible = LegalSlicesForOp(graph, hw, i);
    if (feasible.empty()) {
      throw std::runtime_error("operation '" + graph.operation(i).id +
                               "' has no legal slice");
    }
    for (const int p : feasible) {
      vars->place[i][p] = solver->MakeBoolVar(VarName("y", i, p));
    }
  }

  vars->makespan = solver->MakeIntVar(0.0, infinity, "makespan");
}

void CreateSliceIcuVariables(const Dfg& graph, VariableContext* vars) {
  MPSolver* solver = vars->solver;
  const int n = graph.NumOps();
  for (int i = 0; i < n; ++i) {
    for (int j = i + 1; j < n; ++j) {
      const OpPairKey key{i, j};
      vars->abs_pos_delta[key] =
          solver->MakeIntVar(0.0, solver->infinity(), VarName("abs_pos", i, j));
      vars->abs_issue_delta[key] = solver->MakeIntVar(
          0.0, solver->infinity(), VarName("abs_issue", i, j));
      vars->pos_order[key] = solver->MakeBoolVar(VarName("pos_order", i, j));
      vars->issue_order[key] =
          solver->MakeBoolVar(VarName("issue_order", i, j));
      vars->same_slice_same_cycle[key] =
          solver->MakeBoolVar(VarName("same_slice_cycle", i, j));
    }
  }
}

void CreateStreamVariables(const Dfg& graph, const Hardware& hw,
                           VariableContext* vars) {
  MPSolver* solver = vars->solver;
  const int edge_count = graph.NumEdges();

  vars->east.resize(edge_count);
  vars->west.resize(edge_count);
  vars->locate.resize(edge_count);
  vars->lo.resize(edge_count);
  vars->hi.resize(edge_count);
  vars->phase.resize(edge_count);

  for (int e = 0; e < edge_count; ++e) {
    vars->east[e] = solver->MakeBoolVar(VarName("east", e));
    vars->west[e] = solver->MakeBoolVar(VarName("west", e));
    vars->lo[e] = solver->MakeIntVar(hw.min_slice, hw.max_slice, VarName("lo", e));
    vars->hi[e] = solver->MakeIntVar(hw.min_slice, hw.max_slice, VarName("hi", e));
    vars->phase[e] = solver->MakeIntVar(-hw.big_m, hw.big_m, VarName("phase", e));

    vars->locate[e].resize(hw.stream_count);
    for (int s = 0; s < hw.stream_count; ++s) {
      vars->locate[e][s] = solver->MakeBoolVar(VarName("stream", e, s));
    }
  }

  for (int e1 = 0; e1 < edge_count; ++e1) {
    for (int e2 = e1 + 1; e2 < edge_count; ++e2) {
      const EdgePairKey key{e1, e2};
      vars->same_east[key] = solver->MakeBoolVar(VarName("same_east", e1, e2));
      vars->same_west[key] = solver->MakeBoolVar(VarName("same_west", e1, e2));
      vars->same_stream_dir[key] =
          solver->MakeBoolVar(VarName("same_dir", e1, e2));
      vars->same_stream_id[key] =
          solver->MakeBoolVar(VarName("same_stream_id", e1, e2));
      vars->before_i[key] = solver->MakeBoolVar(VarName("before_i", e1, e2));
      vars->before_ii[key] = solver->MakeBoolVar(VarName("before_ii", e1, e2));
      vars->overlap[key] = solver->MakeBoolVar(VarName("overlap", e1, e2));
      vars->order_delta[key] = solver->MakeBoolVar(VarName("delta", e1, e2));
      for (int s = 0; s < hw.stream_count; ++s) {
        vars->same_stream[{e1, e2, s}] =
            solver->MakeBoolVar(VarName("same_stream", e1, e2, s));
      }
    }
  }
}

}  // namespace dfg_ilp
