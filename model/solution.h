#ifndef MODEL_SOLUTION_H_
#define MODEL_SOLUTION_H_

#include <string>
#include <vector>

namespace dfg_ilp {

enum class SolveStatus {
  kOptimal,
  kFeasible,
  kInfeasible,
  kUnbounded,
  kAbnormal,
  kNotSolved,
};

enum class SolverMode {
  kBaseline,
  kGraphPreprocessed,
};

struct OpAssignment {
  std::string id;
  std::string kind;
  int dfunc = 0;
  int issue = 0;
  int finish = 0;
  int pos = 0;
};

struct EdgeAssignment {
  std::string src;
  std::string dst;
  int stream = -1;
  std::string direction;
  int lo = 0;
  int hi = 0;
  int phase = 0;
};

struct SolveResult {
  SolverMode mode = SolverMode::kBaseline;
  SolveStatus status = SolveStatus::kNotSolved;
  double objective_value = 0.0;
  int makespan = 0;
  int variable_count = 0;
  int constraint_count = 0;
  long long solve_time_ms = 0;
  long long preprocess_time_ms = 0;
  int total_op_pairs = 0;
  int active_op_pairs = 0;
  int pruned_op_pairs = 0;
  double pair_reduction_rate = 0.0;
  double average_issue_window = 0.0;
  bool domain_check_ok = true;
  std::vector<std::string> domain_check_errors;
  std::vector<OpAssignment> operations;
  std::vector<EdgeAssignment> edges;
};

std::string ToString(SolveStatus status);
std::string ToString(SolverMode mode);

}  // namespace dfg_ilp

#endif  // MODEL_SOLUTION_H_
