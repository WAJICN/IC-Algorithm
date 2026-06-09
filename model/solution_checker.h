#ifndef MODEL_SOLUTION_CHECKER_H_
#define MODEL_SOLUTION_CHECKER_H_

#include <string>
#include <vector>

#include "dfg/dfg.h"
#include "model/hardware.h"
#include "model/solution.h"

namespace dfg_ilp {

struct DomainCheckResult {
  bool ok = true;
  std::vector<std::string> errors;
};

DomainCheckResult CheckSolutionDomain(const Dfg& graph, const Hardware& hw,
                                      const SolveResult& result);

}  // namespace dfg_ilp

#endif  // MODEL_SOLUTION_CHECKER_H_
