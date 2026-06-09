#include "model/solution.h"

namespace dfg_ilp {

std::string ToString(SolveStatus status) {
  switch (status) {
    case SolveStatus::kOptimal:
      return "optimal";
    case SolveStatus::kFeasible:
      return "feasible";
    case SolveStatus::kInfeasible:
      return "infeasible";
    case SolveStatus::kUnbounded:
      return "unbounded";
    case SolveStatus::kAbnormal:
      return "abnormal";
    case SolveStatus::kNotSolved:
      return "not_solved";
  }
  return "unknown";
}

std::string ToString(SolverMode mode) {
  switch (mode) {
    case SolverMode::kBaseline:
      return "baseline";
    case SolverMode::kGraphPreprocessed:
      return "graph_preprocessed";
  }
  return "unknown";
}

}  // namespace dfg_ilp
