#include "model/solution_checker.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace dfg_ilp {
namespace {

void AddError(DomainCheckResult* check, const std::string& error) {
  check->ok = false;
  check->errors.push_back(error);
}

std::string EdgeName(const EdgeAssignment& edge) {
  return edge.src + " -> " + edge.dst;
}

std::map<std::string, const OpAssignment*> OperationAssignments(
    const SolveResult& result) {
  std::map<std::string, const OpAssignment*> by_id;
  for (const OpAssignment& op : result.operations) {
    by_id[op.id] = &op;
  }
  return by_id;
}

bool FeasibleSliceContains(const Operation& op, int slice) {
  if (!op.feasible_slices) {
    return true;
  }
  return std::find(op.feasible_slices->begin(), op.feasible_slices->end(),
                   slice) != op.feasible_slices->end();
}

int CycleAtPos(const EdgeAssignment& edge, int pos) {
  if (edge.direction == "east") {
    return edge.phase + pos;
  }
  return edge.phase - pos;
}

bool IntervalsOverlap(int lo_a, int hi_a, int lo_b, int hi_b) {
  return !(hi_a < lo_b || hi_b < lo_a);
}

void CheckOperations(const Dfg& graph, const Hardware& hw,
                     const SolveResult& result,
                     const std::map<std::string, const OpAssignment*>& by_id,
                     DomainCheckResult* check) {
  if (static_cast<int>(result.operations.size()) != graph.NumOps()) {
    AddError(check, "operation assignment count does not match DFG");
  }

  for (int i = 0; i < graph.NumOps(); ++i) {
    const Operation& expected = graph.operation(i);
    const auto it = by_id.find(expected.id);
    if (it == by_id.end()) {
      AddError(check, "missing assignment for operation '" + expected.id + "'");
      continue;
    }
    const OpAssignment& op = *it->second;
    if (op.kind != ToString(expected.kind)) {
      AddError(check, "operation '" + op.id + "' kind mismatch");
    }
    if (op.dfunc != expected.dfunc) {
      AddError(check, "operation '" + op.id + "' dfunc mismatch");
    }
    if (op.finish != op.issue + op.dfunc) {
      AddError(check, "operation '" + op.id + "' finish != issue + dfunc");
    }
    if (!hw.IsHardwareLegalSlice(op.pos)) {
      AddError(check, "operation '" + op.id + "' uses hardware-illegal slice");
    }
    if (!FeasibleSliceContains(expected, op.pos)) {
      AddError(check, "operation '" + op.id + "' violates feasible slice set");
    }
    if (result.makespan < op.finish) {
      AddError(check, "makespan does not cover operation '" + op.id + "'");
    }
  }
}

void CheckTiming(const Dfg& graph,
                 const std::map<std::string, const OpAssignment*>& by_id,
                 const Hardware& hw, DomainCheckResult* check) {
  for (const Edge& edge : graph.edges()) {
    const Operation& src_op = graph.operation(edge.src);
    const Operation& dst_op = graph.operation(edge.dst);
    const auto src_it = by_id.find(src_op.id);
    const auto dst_it = by_id.find(dst_op.id);
    if (src_it == by_id.end() || dst_it == by_id.end()) {
      continue;
    }
    const OpAssignment& src = *src_it->second;
    const OpAssignment& dst = *dst_it->second;
    const int distance = std::abs(src.pos - dst.pos);
    const int required = src.issue + src.dfunc + distance + hw.k_stream;
    const bool exact =
        src_op.kind == OpKind::kCompute || dst_op.kind == OpKind::kCompute;
    if (exact && dst.issue != required) {
      AddError(check, "exact timing violation on edge '" + src.id + " -> " +
                          dst.id + "'");
    }
    if (!exact && dst.issue < required) {
      AddError(check, "lower-bound timing violation on edge '" + src.id +
                          " -> " + dst.id + "'");
    }
  }
}

void CheckNonOverlap(const SolveResult& result, DomainCheckResult* check) {
  for (std::size_t i = 0; i < result.operations.size(); ++i) {
    for (std::size_t j = i + 1; j < result.operations.size(); ++j) {
      const OpAssignment& a = result.operations[i];
      const OpAssignment& b = result.operations[j];
      if (a.pos != b.pos) {
        continue;
      }
      if (a.finish > b.issue && b.finish > a.issue) {
        AddError(check, "same-slice interval overlap between '" + a.id +
                            "' and '" + b.id + "'");
      }
    }
  }
}

void CheckStreams(const Dfg& graph,
                  const std::map<std::string, const OpAssignment*>& by_id,
                  const Hardware& hw, const SolveResult& result,
                  DomainCheckResult* check) {
  if (static_cast<int>(result.edges.size()) != graph.NumEdges()) {
    AddError(check, "edge assignment count does not match DFG");
  }

  for (const EdgeAssignment& edge : result.edges) {
    const auto src_it = by_id.find(edge.src);
    const auto dst_it = by_id.find(edge.dst);
    if (src_it == by_id.end() || dst_it == by_id.end()) {
      AddError(check, "edge assignment references an unknown operation '" +
                          EdgeName(edge) + "'");
      continue;
    }
    const OpAssignment& src = *src_it->second;
    const OpAssignment& dst = *dst_it->second;
    if (edge.stream < 0 || edge.stream >= hw.stream_count) {
      AddError(check, "stream id out of range on edge '" + EdgeName(edge) + "'");
    }
    if (edge.direction != "east" && edge.direction != "west") {
      AddError(check, "invalid stream direction on edge '" + EdgeName(edge) + "'");
      continue;
    }
    if (edge.direction == "east" && dst.pos < src.pos) {
      AddError(check, "east route moves west on edge '" + EdgeName(edge) + "'");
    }
    if (edge.direction == "west" && src.pos < dst.pos) {
      AddError(check, "west route moves east on edge '" + EdgeName(edge) + "'");
    }
    if (edge.lo != std::min(src.pos, dst.pos) ||
        edge.hi != std::max(src.pos, dst.pos)) {
      AddError(check, "route interval mismatch on edge '" + EdgeName(edge) + "'");
    }
    const int expected_phase =
        edge.direction == "east" ? src.issue + src.dfunc - src.pos
                                 : src.issue + src.dfunc + src.pos;
    if (edge.phase != expected_phase) {
      AddError(check, "route phase mismatch on edge '" + EdgeName(edge) + "'");
    }
    const int arrival_cycle = CycleAtPos(edge, dst.pos);
    if (arrival_cycle > dst.issue) {
      AddError(check, "route arrives after consumer issue on edge '" +
                          EdgeName(edge) + "'");
    }
  }

  for (std::size_t i = 0; i < result.edges.size(); ++i) {
    for (std::size_t j = i + 1; j < result.edges.size(); ++j) {
      const EdgeAssignment& a = result.edges[i];
      const EdgeAssignment& b = result.edges[j];
      if (a.stream != b.stream || a.direction != b.direction) {
        continue;
      }
      if (!IntervalsOverlap(a.lo, a.hi, b.lo, b.hi)) {
        continue;
      }
      if (a.phase == b.phase) {
        AddError(check, "stream conflict between edges '" + EdgeName(a) +
                            "' and '" + EdgeName(b) + "'");
      }
    }
  }
}

}  // namespace

DomainCheckResult CheckSolutionDomain(const Dfg& graph, const Hardware& hw,
                                      const SolveResult& result) {
  DomainCheckResult check;
  if (result.status != SolveStatus::kOptimal &&
      result.status != SolveStatus::kFeasible) {
    return check;
  }

  const std::map<std::string, const OpAssignment*> by_id =
      OperationAssignments(result);
  CheckOperations(graph, hw, result, by_id, &check);
  CheckTiming(graph, by_id, hw, &check);
  CheckNonOverlap(result, &check);
  CheckStreams(graph, by_id, hw, result, &check);
  return check;
}

}  // namespace dfg_ilp
