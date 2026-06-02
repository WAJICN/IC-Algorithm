#ifndef VISUALIZATION_EXPORTER_H_
#define VISUALIZATION_EXPORTER_H_

#include <string>

#include "model/solution.h"

namespace dfg_ilp {

bool WriteSolutionJson(const SolveResult& result, const std::string& path,
                       std::string* error);
bool WriteSolutionCsv(const SolveResult& result, const std::string& path,
                      std::string* error);

}  // namespace dfg_ilp

#endif  // VISUALIZATION_EXPORTER_H_
