#ifndef DFG_VALIDATOR_H_
#define DFG_VALIDATOR_H_

#include <string>
#include <vector>

#include "dfg/dfg.h"

namespace dfg_ilp {

struct ValidationResult {
  bool ok = false;
  std::vector<std::string> errors;
};

ValidationResult ValidateDfg(const Dfg& graph);

}  // namespace dfg_ilp

#endif  // DFG_VALIDATOR_H_
