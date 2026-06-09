#include <iostream>
#include <set>

#include "dfg/parser.h"
#include "model/hardware.h"
#include "model/preprocess.h"

namespace {

bool ContainsPair(const std::vector<dfg_ilp::OpPairKey>& pairs, int first,
                  int second) {
  for (const dfg_ilp::OpPairKey& pair : pairs) {
    if (pair.first == first && pair.second == second) {
      return true;
    }
  }
  return false;
}

}  // namespace

int main() {
  const char* text = R"(
op a compute 1 slices=0
op b compute 2 slices=2
op c compute 1 slices=5
op d compute 1 slices=5
edge a b
edge a c
edge b d
edge c d
)";
  const dfg_ilp::ParseResult parse = dfg_ilp::ParseDfgText(text, "inline");
  if (!parse.ok) {
    std::cerr << parse.error << '\n';
    return 1;
  }

  dfg_ilp::Hardware hw;
  hw.max_issue_time = 30;
  const dfg_ilp::GraphPreprocessResult result =
      dfg_ilp::PreprocessGraph(parse.graph, hw);

  if (result.topological_order.size() != 4) {
    std::cerr << "topological order size mismatch\n";
    return 1;
  }
  if (std::set<int>(result.topological_order.begin(),
                    result.topological_order.end()).size() != 4) {
    std::cerr << "topological order does not contain each op once\n";
    return 1;
  }

  if (dfg_ilp::MinimumFeasibleSliceDistance(parse.graph, hw, 0, 1) != 2 ||
      dfg_ilp::MinimumFeasibleSliceDistance(parse.graph, hw, 1, 3) != 3) {
    std::cerr << "minimum feasible slice distance mismatch\n";
    return 1;
  }

  if (result.asap_issue.size() != 4 || result.alap_issue.size() != 4) {
    std::cerr << "bound vector size mismatch\n";
    return 1;
  }
  if (result.asap_issue[0] != 0 || result.asap_issue[1] != 4 ||
      result.asap_issue[2] != 7 || result.asap_issue[3] != 10) {
    std::cerr << "ASAP bounds mismatch\n";
    return 1;
  }
  if (result.alap_issue[3] != 30 || result.alap_issue[1] != 24 ||
      result.alap_issue[2] != 28 || result.alap_issue[0] != 20) {
    std::cerr << "ALAP bounds mismatch\n";
    return 1;
  }
  for (std::size_t i = 0; i < result.asap_issue.size(); ++i) {
    if (result.asap_issue[i] > result.alap_issue[i]) {
      std::cerr << "empty issue window\n";
      return 1;
    }
  }

  if (!result.reachable[0][1] || !result.reachable[0][2] ||
      !result.reachable[0][3] || !result.reachable[1][3] ||
      !result.reachable[2][3]) {
    std::cerr << "reachability mismatch\n";
    return 1;
  }
  if (result.reachable[1][2] || result.reachable[2][1]) {
    std::cerr << "incomparable ops marked reachable\n";
    return 1;
  }

  if (result.total_op_pairs != 6 || result.active_op_pairs != 1 ||
      result.pruned_op_pairs != 5 || !ContainsPair(result.incomparable_pairs, 1, 2)) {
    std::cerr << "incomparable pair metrics mismatch\n";
    return 1;
  }

  return 0;
}
