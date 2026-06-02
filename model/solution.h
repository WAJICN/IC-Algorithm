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
  SolveStatus status = SolveStatus::kNotSolved;
  double objective_value = 0.0;
  int makespan = 0;
  int variable_count = 0;
  int constraint_count = 0;
  std::vector<OpAssignment> operations;
  std::vector<EdgeAssignment> edges;
};

std::string ToString(SolveStatus status);

}  // namespace dfg_ilp

#endif  // MODEL_SOLUTION_H_
