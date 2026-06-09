#include <iostream>
#include <map>
#include <string>

#include "dfg/parser.h"
#include "model/hardware.h"
#include "model/ilp_solver.h"

bool IsSolved(const dfg_ilp::SolveResult& result) {
  return result.status == dfg_ilp::SolveStatus::kOptimal ||
         result.status == dfg_ilp::SolveStatus::kFeasible;
}

int main() {
  const char* text = R"(
op a compute 1 slices=-1..1
op b compute 1 slices=-1..1
op c compute 1 slices=-1..1
edge a b
edge a c
)";
  const dfg_ilp::ParseResult parse = dfg_ilp::ParseDfgText(text, "inline");
  if (!parse.ok) {
    std::cerr << parse.error << '\n';
    return 1;
  }

  dfg_ilp::Hardware hardware;
  hardware.max_issue_time = 20;
  dfg_ilp::SolverConfig baseline_config;
  baseline_config.time_limit_ms = 5000;
  baseline_config.mode = dfg_ilp::SolverMode::kBaseline;

  dfg_ilp::IlpSolver baseline_solver(hardware, baseline_config);
  const dfg_ilp::SolveResult result = baseline_solver.Solve(parse.graph);
  if (!IsSolved(result)) {
    std::cerr << "unexpected baseline solve status: "
              << dfg_ilp::ToString(result.status)
              << '\n';
    return 1;
  }
  if (result.operations.size() != 3 || result.makespan <= 0) {
    std::cerr << "invalid solution shape\n";
    return 1;
  }
  if (result.mode != dfg_ilp::SolverMode::kBaseline ||
      result.total_op_pairs != 3 || result.active_op_pairs != 3 ||
      result.pruned_op_pairs != 0 || !result.domain_check_ok) {
    std::cerr << "invalid baseline metrics\n";
    return 1;
  }

  dfg_ilp::SolverConfig graph_config;
  graph_config.time_limit_ms = 5000;
  graph_config.mode = dfg_ilp::SolverMode::kGraphPreprocessed;
  dfg_ilp::IlpSolver graph_solver(hardware, graph_config);
  const dfg_ilp::SolveResult graph_result = graph_solver.Solve(parse.graph);
  if (!IsSolved(graph_result)) {
    std::cerr << "unexpected graph solve status: "
              << dfg_ilp::ToString(graph_result.status) << '\n';
    return 1;
  }
  if (graph_result.mode != dfg_ilp::SolverMode::kGraphPreprocessed ||
      graph_result.total_op_pairs != 3 || graph_result.active_op_pairs > result.active_op_pairs ||
      !graph_result.domain_check_ok) {
    std::cerr << "invalid graph preprocessing metrics\n";
    return 1;
  }

  for (std::size_t i = 0; i < graph_result.operations.size(); ++i) {
    for (std::size_t j = i + 1; j < graph_result.operations.size(); ++j) {
      const dfg_ilp::OpAssignment& a = graph_result.operations[i];
      const dfg_ilp::OpAssignment& b = graph_result.operations[j];
      if (a.pos == b.pos && a.finish > b.issue && b.finish > a.issue) {
        std::cerr << "same-slice interval overlap in graph solution\n";
        return 1;
      }
    }
  }

  std::map<std::string, dfg_ilp::OpAssignment> by_id;
  for (const dfg_ilp::OpAssignment& op : result.operations) {
    by_id[op.id] = op;
  }
  for (const dfg_ilp::EdgeAssignment& edge : result.edges) {
    const dfg_ilp::OpAssignment& src = by_id.at(edge.src);
    const dfg_ilp::OpAssignment& dst = by_id.at(edge.dst);
    const int expected_lo = std::min(src.pos, dst.pos);
    const int expected_hi = std::max(src.pos, dst.pos);
    if (edge.lo != expected_lo || edge.hi != expected_hi) {
      std::cerr << "route interval mismatch\n";
      return 1;
    }
    const int expected_phase =
        edge.direction == "east" ? src.issue + src.dfunc - src.pos
                                 : src.issue + src.dfunc + src.pos;
    if (edge.phase != expected_phase) {
      std::cerr << "route phase mismatch\n";
      return 1;
    }
  }
  return 0;
}
