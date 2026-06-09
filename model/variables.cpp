#include "model/variables.h"

#include <algorithm>
#include <set>
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
                         VariableContext* vars,
                         const std::vector<int>* issue_lower_bounds,
                         const std::vector<int>* issue_upper_bounds) {
  MPSolver* solver = vars->solver;
  const double infinity = solver->infinity();
  const int n = graph.NumOps();

  if (issue_lower_bounds && static_cast<int>(issue_lower_bounds->size()) != n) {
    throw std::runtime_error("issue lower-bound vector size does not match DFG");
  }
  if (issue_upper_bounds && static_cast<int>(issue_upper_bounds->size()) != n) {
    throw std::runtime_error("issue upper-bound vector size does not match DFG");
  }

  vars->issue.resize(n);
  vars->pos.resize(n);
  vars->place.resize(n);

  for (int i = 0; i < n; ++i) {
    const int issue_lb = issue_lower_bounds ? issue_lower_bounds->at(i) : 0;
    const int issue_ub =
        issue_upper_bounds ? issue_upper_bounds->at(i) : hw.max_issue_time;
    if (issue_lb > issue_ub) {
      throw std::runtime_error("empty issue window for operation '" +
                               graph.operation(i).id + "'");
    }
    vars->issue[i] =
        solver->MakeIntVar(issue_lb, issue_ub, VarName("issue", i));
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

void CreateSliceIcuVariables(const std::vector<OpPairKey>& distance_pairs,
                             const std::vector<OpPairKey>& resource_pairs,
                             VariableContext* vars) {
  MPSolver* solver = vars->solver;
  for (const OpPairKey& key : distance_pairs) {
    vars->abs_pos_delta[key] = solver->MakeIntVar(
        0.0, solver->infinity(), VarName("abs_pos", key.first, key.second));
    vars->pos_order[key] =
        solver->MakeBoolVar(VarName("pos_order", key.first, key.second));
  }
  for (const OpPairKey& key : resource_pairs) {
    vars->op_interval_order[key] =
        solver->MakeBoolVar(VarName("op_before", key.first, key.second));
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

std::vector<OpPairKey> AllOperationPairs(const Dfg& graph) {
  std::vector<OpPairKey> pairs;
  const int n = graph.NumOps();
  pairs.reserve(n * (n - 1) / 2);
  for (int i = 0; i < n; ++i) {
    for (int j = i + 1; j < n; ++j) {
      pairs.push_back(OpPairKey{i, j});
    }
  }
  return pairs;
}

std::vector<OpPairKey> EdgeEndpointPairs(const Dfg& graph) {
  std::vector<OpPairKey> pairs;
  pairs.reserve(graph.NumEdges());
  for (const Edge& edge : graph.edges()) {
    pairs.push_back(OpPairKey{std::min(edge.src, edge.dst),
                              std::max(edge.src, edge.dst)});
  }
  return MergeOperationPairs(pairs, {});
}

std::vector<OpPairKey> MergeOperationPairs(const std::vector<OpPairKey>& first,
                                           const std::vector<OpPairKey>& second) {
  std::set<OpPairKey> unique_pairs;
  unique_pairs.insert(first.begin(), first.end());
  unique_pairs.insert(second.begin(), second.end());
  return std::vector<OpPairKey>(unique_pairs.begin(), unique_pairs.end());
}

std::vector<OpPairKey> SafeGraphResourcePairs(
    const Dfg& graph, const std::vector<OpPairKey>& incomparable_pairs) {
  std::vector<OpPairKey> pairs = incomparable_pairs;
  for (const Edge& edge : graph.edges()) {
    const Operation& src = graph.operation(edge.src);
    const Operation& dst = graph.operation(edge.dst);
    if (src.kind == OpKind::kMemory && dst.kind == OpKind::kMemory) {
      pairs.push_back(OpPairKey{std::min(edge.src, edge.dst),
                                std::max(edge.src, edge.dst)});
    }
  }
  return MergeOperationPairs(pairs, {});
}

}  // namespace dfg_ilp
