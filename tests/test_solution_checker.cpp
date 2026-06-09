#include <iostream>

#include "dfg/parser.h"
#include "model/hardware.h"
#include "model/solution_checker.h"

int main() {
  const char* text = R"(
op a compute 2 slices=0
op b compute 2 slices=0
edge a b
)";
  const dfg_ilp::ParseResult parse = dfg_ilp::ParseDfgText(text, "inline");
  if (!parse.ok) {
    std::cerr << parse.error << '\n';
    return 1;
  }

  dfg_ilp::SolveResult good;
  good.status = dfg_ilp::SolveStatus::kOptimal;
  good.makespan = 6;
  good.operations.push_back({"a", "compute", 2, 0, 2, 0});
  good.operations.push_back({"b", "compute", 2, 3, 5, 0});
  good.edges.push_back({"a", "b", 0, "east", 0, 0, 2});
  const dfg_ilp::DomainCheckResult good_check =
      dfg_ilp::CheckSolutionDomain(parse.graph, dfg_ilp::Hardware{}, good);
  if (!good_check.ok) {
    std::cerr << "good solution failed domain check\n";
    for (const std::string& error : good_check.errors) {
      std::cerr << error << '\n';
    }
    return 1;
  }

  dfg_ilp::SolveResult bad = good;
  bad.operations[1].issue = 1;
  bad.operations[1].finish = 3;
  bad.edges[0] = {"a", "b", 0, "east", 0, 0, 2};
  const dfg_ilp::DomainCheckResult bad_check =
      dfg_ilp::CheckSolutionDomain(parse.graph, dfg_ilp::Hardware{}, bad);
  if (bad_check.ok) {
    std::cerr << "bad solution should fail domain check\n";
    return 1;
  }

  return 0;
}
