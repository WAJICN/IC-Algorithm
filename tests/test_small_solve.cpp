#include <iostream>
#include <map>
#include <string>

#include "dfg/parser.h"
#include "model/hardware.h"
#include "model/ilp_solver.h"

int main() {
  const char* text = R"(
op a compute 1 slices=-1..1
op b compute 1 slices=-1..1
edge a b
)";
  const dfg_ilp::ParseResult parse = dfg_ilp::ParseDfgText(text, "inline");
  if (!parse.ok) {
    std::cerr << parse.error << '\n';
    return 1;
  }

  dfg_ilp::Hardware hardware;
  hardware.max_issue_time = 20;
  dfg_ilp::SolverConfig config;
  config.time_limit_ms = 5000;

  dfg_ilp::IlpSolver solver(hardware, config);
  const dfg_ilp::SolveResult result = solver.Solve(parse.graph);
  if (result.status != dfg_ilp::SolveStatus::kOptimal &&
      result.status != dfg_ilp::SolveStatus::kFeasible) {
    std::cerr << "unexpected solve status: " << dfg_ilp::ToString(result.status)
              << '\n';
    return 1;
  }
  if (result.operations.size() != 2 || result.makespan <= 0) {
    std::cerr << "invalid solution shape\n";
    return 1;
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
