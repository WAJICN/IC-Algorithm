#ifndef DFG_DFG_H_
#define DFG_DFG_H_

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace dfg_ilp {

enum class OpKind {
  kCompute,
  kMemory,
};

struct Operation {
  std::string id;
  OpKind kind = OpKind::kCompute;
  int dfunc = 0;
  std::optional<std::vector<int>> feasible_slices;
};

struct Edge {
  int src = -1;
  int dst = -1;
};

class Dfg {
 public:
  int AddOperation(Operation op);
  void AddEdgeByIndex(int src, int dst);

  const std::vector<Operation>& operations() const { return operations_; }
  const std::vector<Edge>& edges() const { return edges_; }

  int NumOps() const { return static_cast<int>(operations_.size()); }
  int NumEdges() const { return static_cast<int>(edges_.size()); }

  int FindOperationIndex(const std::string& id) const;
  const Operation& operation(int index) const { return operations_.at(index); }
  const Edge& edge(int index) const { return edges_.at(index); }

 private:
  std::vector<Operation> operations_;
  std::vector<Edge> edges_;
  std::unordered_map<std::string, int> id_to_index_;
};

std::string ToString(OpKind kind);
bool ParseOpKind(const std::string& text, OpKind* kind);

}  // namespace dfg_ilp

#endif  // DFG_DFG_H_
