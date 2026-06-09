#ifndef MODEL_ILP_SOLVER_H_
#define MODEL_ILP_SOLVER_H_

#include <string>

#include "dfg/dfg.h"
#include "model/hardware.h"
#include "model/solution.h"

namespace dfg_ilp {

struct SolverConfig {
  std::string backend = "SCIP";
  int time_limit_ms = 30000;
  bool verify_solution = true;
  SolverMode mode = SolverMode::kBaseline;
};

class IlpSolver {
 public:
  IlpSolver(Hardware hardware, SolverConfig config);

  SolveResult Solve(const Dfg& graph) const;

 private:
  Hardware hardware_;
  SolverConfig config_;
};

}  // namespace dfg_ilp

#endif  // MODEL_ILP_SOLVER_H_
