#include <iostream>

#include "dfg/parser.h"

int main() {
  const char* text = R"(
op a compute 1 slices=-2..2
op b memory 3 slices=-1,4..5
edge a b
)";
  const dfg_ilp::ParseResult result = dfg_ilp::ParseDfgText(text, "inline");
  if (!result.ok) {
    std::cerr << result.error << '\n';
    return 1;
  }
  if (result.graph.NumOps() != 2 || result.graph.NumEdges() != 1) {
    std::cerr << "unexpected graph size\n";
    return 1;
  }
  if (!result.graph.operation(1).feasible_slices ||
      result.graph.operation(1).feasible_slices->size() != 3) {
    std::cerr << "slice spec was not parsed\n";
    return 1;
  }
  return 0;
}
