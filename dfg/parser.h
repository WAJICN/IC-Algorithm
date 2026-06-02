#ifndef DFG_PARSER_H_
#define DFG_PARSER_H_

#include <string>

#include "dfg/dfg.h"

namespace dfg_ilp {

struct ParseResult {
  bool ok = false;
  Dfg graph;
  std::string error;
};

ParseResult ParseDfgFile(const std::string& path);
ParseResult ParseDfgText(const std::string& text, const std::string& source_name);

}  // namespace dfg_ilp

#endif  // DFG_PARSER_H_
