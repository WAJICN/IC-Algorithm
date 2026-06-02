#include <iostream>

#include "dfg/parser.h"
#include "dfg/validator.h"

int main() {
  const char* text = R"(
op a compute 1
op b compute 1
edge a b
edge b a
)";
  const dfg_ilp::ParseResult parse = dfg_ilp::ParseDfgText(text, "inline");
  if (!parse.ok) {
    std::cerr << parse.error << '\n';
    return 1;
  }
  const dfg_ilp::ValidationResult validation = dfg_ilp::ValidateDfg(parse.graph);
  if (validation.ok) {
    std::cerr << "cycle should be rejected\n";
    return 1;
  }
  return 0;
}
