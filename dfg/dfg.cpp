#include "dfg/dfg.h"

#include <algorithm>
#include <cctype>

namespace dfg_ilp {

int Dfg::AddOperation(Operation op) {
  const int index = static_cast<int>(operations_.size());
  id_to_index_.emplace(op.id, index);
  operations_.push_back(std::move(op));
  return index;
}

void Dfg::AddEdgeByIndex(int src, int dst) {
  edges_.push_back(Edge{src, dst});
}

int Dfg::FindOperationIndex(const std::string& id) const {
  const auto it = id_to_index_.find(id);
  if (it == id_to_index_.end()) {
    return -1;
  }
  return it->second;
}

std::string ToString(OpKind kind) {
  switch (kind) {
    case OpKind::kCompute:
      return "compute";
    case OpKind::kMemory:
      return "memory";
  }
  return "unknown";
}

bool ParseOpKind(const std::string& text, OpKind* kind) {
  std::string normalized = text;
  std::transform(normalized.begin(), normalized.end(), normalized.begin(),
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  if (normalized == "comp" || normalized == "compute") {
    *kind = OpKind::kCompute;
    return true;
  }
  if (normalized == "mem" || normalized == "memory") {
    *kind = OpKind::kMemory;
    return true;
  }
  return false;
}

}  // namespace dfg_ilp
