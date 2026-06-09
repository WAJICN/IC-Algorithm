#include "model/preprocess.h"

#include <algorithm>
#include <cstdlib>
#include <queue>
#include <stdexcept>
#include <string>
#include <vector>

namespace dfg_ilp {
namespace {

std::vector<int> LegalSlicesForOp(const Dfg& graph, const Hardware& hw,
                                  int op_index) {
  const Operation& op = graph.operation(op_index);
  if (!op.feasible_slices) {
    return hw.AllSlices();
  }
  return hw.IntersectLegalSlices(*op.feasible_slices);
}

std::vector<std::vector<int>> BuildAdjacency(const Dfg& graph) {
  std::vector<std::vector<int>> adjacency(graph.NumOps());
  for (const Edge& edge : graph.edges()) {
    adjacency[edge.src].push_back(edge.dst);
  }
  return adjacency;
}

std::vector<int> ComputeTopologicalOrder(const Dfg& graph) {
  const int n = graph.NumOps();
  std::vector<int> indegree(n, 0);
  std::vector<std::vector<int>> adjacency(n);
  for (const Edge& edge : graph.edges()) {
    adjacency[edge.src].push_back(edge.dst);
    ++indegree[edge.dst];
  }

  std::queue<int> ready;
  for (int i = 0; i < n; ++i) {
    if (indegree[i] == 0) {
      ready.push(i);
    }
  }

  std::vector<int> order;
  order.reserve(n);
  while (!ready.empty()) {
    const int current = ready.front();
    ready.pop();
    order.push_back(current);
    for (const int next : adjacency[current]) {
      --indegree[next];
      if (indegree[next] == 0) {
        ready.push(next);
      }
    }
  }

  if (static_cast<int>(order.size()) != n) {
    throw std::runtime_error("cannot preprocess a cyclic DFG");
  }
  return order;
}

int MinLatencyForEdge(const Dfg& graph, const Hardware& hw, const Edge& edge) {
  return graph.operation(edge.src).dfunc + hw.k_stream +
         MinimumFeasibleSliceDistance(graph, hw, edge.src, edge.dst);
}

std::vector<int> ComputeAsap(const Dfg& graph, const Hardware& hw,
                             const std::vector<int>& topological_order) {
  std::vector<int> asap(graph.NumOps(), 0);
  std::vector<std::vector<int>> outgoing_edges(graph.NumOps());
  for (int e = 0; e < graph.NumEdges(); ++e) {
    outgoing_edges[graph.edge(e).src].push_back(e);
  }

  for (const int op_index : topological_order) {
    for (const int edge_index : outgoing_edges[op_index]) {
      const Edge& edge = graph.edge(edge_index);
      const int candidate =
          asap[edge.src] + MinLatencyForEdge(graph, hw, edge);
      asap[edge.dst] = std::max(asap[edge.dst], candidate);
    }
  }
  return asap;
}

std::vector<int> ComputeAlap(const Dfg& graph, const Hardware& hw,
                             const std::vector<int>& topological_order) {
  std::vector<int> alap(graph.NumOps(), hw.max_issue_time);
  std::vector<std::vector<int>> outgoing_edges(graph.NumOps());
  for (int e = 0; e < graph.NumEdges(); ++e) {
    outgoing_edges[graph.edge(e).src].push_back(e);
  }

  for (auto it = topological_order.rbegin(); it != topological_order.rend();
       ++it) {
    const int op_index = *it;
    if (outgoing_edges[op_index].empty()) {
      continue;
    }
    int best = hw.max_issue_time;
    for (const int edge_index : outgoing_edges[op_index]) {
      const Edge& edge = graph.edge(edge_index);
      const int candidate =
          alap[edge.dst] - MinLatencyForEdge(graph, hw, edge);
      best = std::min(best, candidate);
    }
    alap[op_index] = best;
  }
  return alap;
}

std::vector<std::vector<bool>> ComputeReachability(
    const Dfg& graph, const std::vector<int>& topological_order) {
  const int n = graph.NumOps();
  const std::vector<std::vector<int>> adjacency = BuildAdjacency(graph);
  std::vector<std::vector<bool>> reachable(n, std::vector<bool>(n, false));

  for (auto it = topological_order.rbegin(); it != topological_order.rend();
       ++it) {
    const int src = *it;
    for (const int dst : adjacency[src]) {
      reachable[src][dst] = true;
      for (int next = 0; next < n; ++next) {
        if (reachable[dst][next]) {
          reachable[src][next] = true;
        }
      }
    }
  }
  return reachable;
}

std::vector<OpPairKey> ComputeIncomparablePairs(
    const std::vector<std::vector<bool>>& reachable) {
  const int n = static_cast<int>(reachable.size());
  std::vector<OpPairKey> pairs;
  for (int i = 0; i < n; ++i) {
    for (int j = i + 1; j < n; ++j) {
      if (!reachable[i][j] && !reachable[j][i]) {
        pairs.push_back(OpPairKey{i, j});
      }
    }
  }
  return pairs;
}

void ValidateBounds(const Dfg& graph, const std::vector<int>& asap,
                    const std::vector<int>& alap) {
  for (int i = 0; i < graph.NumOps(); ++i) {
    if (asap[i] > alap[i]) {
      throw std::runtime_error("preprocessing produced an empty issue window for '" +
                               graph.operation(i).id + "'");
    }
  }
}

double AverageIssueWindow(const std::vector<int>& asap,
                          const std::vector<int>& alap) {
  if (asap.empty()) {
    return 0.0;
  }
  long long total = 0;
  for (std::size_t i = 0; i < asap.size(); ++i) {
    total += alap[i] - asap[i];
  }
  return static_cast<double>(total) / static_cast<double>(asap.size());
}

}  // namespace

int MinimumFeasibleSliceDistance(const Dfg& graph, const Hardware& hw,
                                 int src_index, int dst_index) {
  const std::vector<int> src_slices = LegalSlicesForOp(graph, hw, src_index);
  const std::vector<int> dst_slices = LegalSlicesForOp(graph, hw, dst_index);
  if (src_slices.empty() || dst_slices.empty()) {
    throw std::runtime_error("cannot compute distance for operation with no legal slice");
  }

  int best = std::abs(src_slices.front() - dst_slices.front());
  std::size_t i = 0;
  std::size_t j = 0;
  while (i < src_slices.size() && j < dst_slices.size()) {
    best = std::min(best, std::abs(src_slices[i] - dst_slices[j]));
    if (best == 0) {
      return 0;
    }
    if (src_slices[i] < dst_slices[j]) {
      ++i;
    } else {
      ++j;
    }
  }
  return best;
}

GraphPreprocessResult PreprocessGraph(const Dfg& graph, const Hardware& hw) {
  GraphPreprocessResult result;
  result.topological_order = ComputeTopologicalOrder(graph);
  result.asap_issue = ComputeAsap(graph, hw, result.topological_order);
  result.alap_issue = ComputeAlap(graph, hw, result.topological_order);
  ValidateBounds(graph, result.asap_issue, result.alap_issue);
  result.reachable = ComputeReachability(graph, result.topological_order);
  result.incomparable_pairs = ComputeIncomparablePairs(result.reachable);

  const int n = graph.NumOps();
  result.total_op_pairs = n * (n - 1) / 2;
  result.active_op_pairs = static_cast<int>(result.incomparable_pairs.size());
  result.pruned_op_pairs = result.total_op_pairs - result.active_op_pairs;
  if (result.total_op_pairs > 0) {
    result.pair_reduction_rate =
        static_cast<double>(result.pruned_op_pairs) /
        static_cast<double>(result.total_op_pairs);
  }
  result.average_issue_window =
      AverageIssueWindow(result.asap_issue, result.alap_issue);
  return result;
}

}  // namespace dfg_ilp
