#ifndef MODEL_PREPROCESS_H_
#define MODEL_PREPROCESS_H_

#include <vector>

#include "dfg/dfg.h"
#include "model/hardware.h"
#include "model/variables.h"

namespace dfg_ilp {

struct GraphPreprocessResult {
  std::vector<int> topological_order;
  std::vector<int> asap_issue;
  std::vector<int> alap_issue;
  std::vector<std::vector<bool>> reachable;
  std::vector<OpPairKey> incomparable_pairs;

  int total_op_pairs = 0;
  int active_op_pairs = 0;
  int pruned_op_pairs = 0;
  double pair_reduction_rate = 0.0;
  double average_issue_window = 0.0;
};

int MinimumFeasibleSliceDistance(const Dfg& graph, const Hardware& hw,
                                 int src_index, int dst_index);

GraphPreprocessResult PreprocessGraph(const Dfg& graph, const Hardware& hw);

}  // namespace dfg_ilp

#endif  // MODEL_PREPROCESS_H_
