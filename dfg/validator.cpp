#include "dfg/validator.h"

#include <queue>
#include <string>
#include <unordered_set>
#include <vector>

namespace dfg_ilp {

ValidationResult ValidateDfg(const Dfg& graph) {
  ValidationResult result;
  const int n = graph.NumOps();

  if (n == 0) {
    result.errors.push_back("DFG contains no operations");
  }

  std::unordered_set<std::string> ids;
  for (const Operation& op : graph.operations()) {
    if (op.id.empty()) {
      result.errors.push_back("operation id must not be empty");
    }
    if (!ids.insert(op.id).second) {
      result.errors.push_back("duplicate operation id: " + op.id);
    }
    if (op.dfunc < 0) {
      result.errors.push_back("operation '" + op.id + "' has negative dfunc");
    }
    if (op.feasible_slices && op.feasible_slices->empty()) {
      result.errors.push_back("operation '" + op.id +
                              "' has an empty feasible slice set");
    }
  }

  std::vector<int> indegree(n, 0);
  std::vector<std::vector<int>> adjacency(n);
  std::unordered_set<std::string> edge_keys;
  for (const Edge& edge : graph.edges()) {
    if (edge.src < 0 || edge.src >= n || edge.dst < 0 || edge.dst >= n) {
      result.errors.push_back("edge has an endpoint outside the operation list");
      continue;
    }
    if (edge.src == edge.dst) {
      result.errors.push_back("self edge on operation '" +
                              graph.operation(edge.src).id + "'");
      continue;
    }
    const std::string key =
        std::to_string(edge.src) + "->" + std::to_string(edge.dst);
    if (!edge_keys.insert(key).second) {
      result.errors.push_back("duplicate edge '" + graph.operation(edge.src).id +
                              " -> " + graph.operation(edge.dst).id + "'");
      continue;
    }
    adjacency[edge.src].push_back(edge.dst);
    ++indegree[edge.dst];
  }

  std::queue<int> ready;
  for (int i = 0; i < n; ++i) {
    if (indegree[i] == 0) {
      ready.push(i);
    }
  }

  int visited = 0;
  while (!ready.empty()) {
    const int current = ready.front();
    ready.pop();
    ++visited;
    for (const int next : adjacency[current]) {
      --indegree[next];
      if (indegree[next] == 0) {
        ready.push(next);
      }
    }
  }

  if (visited != n) {
    result.errors.push_back("DFG must be acyclic");
  }

  result.ok = result.errors.empty();
  return result;
}

}  // namespace dfg_ilp
