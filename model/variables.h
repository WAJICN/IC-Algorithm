#ifndef MODEL_VARIABLES_H_
#define MODEL_VARIABLES_H_

#include <map>
#include <memory>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "dfg/dfg.h"
#include "model/hardware.h"
#include "ortools/linear_solver/linear_solver.h"

namespace dfg_ilp {

using operations_research::MPVariable;
using operations_research::MPSolver;

struct EdgePairKey {
  int first = -1;
  int second = -1;

  bool operator<(const EdgePairKey& other) const {
    return std::tie(first, second) < std::tie(other.first, other.second);
  }
};

struct OpPairKey {
  int first = -1;
  int second = -1;

  bool operator<(const OpPairKey& other) const {
    return std::tie(first, second) < std::tie(other.first, other.second);
  }
};

struct VariableContext {
  explicit VariableContext(MPSolver* solver_in) : solver(solver_in) {}

  MPSolver* solver = nullptr;

  std::vector<MPVariable*> issue;
  std::vector<MPVariable*> pos;
  std::vector<std::map<int, MPVariable*>> place;
  MPVariable* makespan = nullptr;

  std::map<OpPairKey, MPVariable*> abs_pos_delta;
  std::map<OpPairKey, MPVariable*> abs_issue_delta;
  std::map<OpPairKey, MPVariable*> pos_order;
  std::map<OpPairKey, MPVariable*> issue_order;
  std::map<OpPairKey, MPVariable*> same_slice_same_cycle;
  std::map<OpPairKey, MPVariable*> op_interval_order;

  std::vector<MPVariable*> east;
  std::vector<MPVariable*> west;
  std::vector<std::vector<MPVariable*>> locate;
  std::vector<MPVariable*> lo;
  std::vector<MPVariable*> hi;
  std::vector<MPVariable*> phase;

  std::map<EdgePairKey, MPVariable*> same_east;
  std::map<EdgePairKey, MPVariable*> same_west;
  std::map<EdgePairKey, MPVariable*> same_stream_dir;
  std::map<EdgePairKey, MPVariable*> same_stream_id;
  std::map<EdgePairKey, MPVariable*> before_i;
  std::map<EdgePairKey, MPVariable*> before_ii;
  std::map<EdgePairKey, MPVariable*> overlap;
  std::map<EdgePairKey, MPVariable*> order_delta;
  std::map<std::tuple<int, int, int>, MPVariable*> same_stream;

  std::vector<int> FeasibleSlicesForOp(const Dfg& graph, const Hardware& hw,
                                       int op_index) const;
};

void CreateCoreVariables(const Dfg& graph, const Hardware& hw,
                         VariableContext* vars,
                         const std::vector<int>* issue_lower_bounds = nullptr,
                         const std::vector<int>* issue_upper_bounds = nullptr);
void CreateSliceIcuVariables(const std::vector<OpPairKey>& distance_pairs,
                             const std::vector<OpPairKey>& resource_pairs,
                             VariableContext* vars);
void CreateStreamVariables(const Dfg& graph, const Hardware& hw,
                           VariableContext* vars);

std::vector<OpPairKey> AllOperationPairs(const Dfg& graph);
std::vector<OpPairKey> EdgeEndpointPairs(const Dfg& graph);
std::vector<OpPairKey> MergeOperationPairs(const std::vector<OpPairKey>& first,
                                           const std::vector<OpPairKey>& second);
std::vector<OpPairKey> SafeGraphResourcePairs(
    const Dfg& graph, const std::vector<OpPairKey>& incomparable_pairs);

std::string VarName(const std::string& prefix, int a);
std::string VarName(const std::string& prefix, int a, int b);
std::string VarName(const std::string& prefix, int a, int b, int c);

}  // namespace dfg_ilp

#endif  // MODEL_VARIABLES_H_
