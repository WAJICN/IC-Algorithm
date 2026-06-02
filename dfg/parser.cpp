#include "dfg/parser.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>

namespace dfg_ilp {
namespace {

std::string Trim(const std::string& s) {
  std::size_t begin = 0;
  while (begin < s.size() && std::isspace(static_cast<unsigned char>(s[begin]))) {
    ++begin;
  }
  std::size_t end = s.size();
  while (end > begin && std::isspace(static_cast<unsigned char>(s[end - 1]))) {
    --end;
  }
  return s.substr(begin, end - begin);
}

std::vector<std::string> SplitWhitespace(const std::string& line) {
  std::istringstream iss(line);
  std::vector<std::string> tokens;
  std::string token;
  while (iss >> token) {
    tokens.push_back(token);
  }
  return tokens;
}

bool ParseInt(const std::string& text, int* value) {
  try {
    std::size_t consumed = 0;
    const int parsed = std::stoi(text, &consumed, 10);
    if (consumed != text.size()) {
      return false;
    }
    *value = parsed;
    return true;
  } catch (...) {
    return false;
  }
}

std::vector<std::string> SplitByComma(const std::string& text) {
  std::vector<std::string> parts;
  std::string current;
  std::istringstream iss(text);
  while (std::getline(iss, current, ',')) {
    parts.push_back(Trim(current));
  }
  return parts;
}

bool ParseSliceSpec(const std::string& spec, std::vector<int>* slices,
                    std::string* error) {
  std::unordered_set<int> seen;
  std::vector<int> parsed;
  for (const std::string& raw_part : SplitByComma(spec)) {
    if (raw_part.empty()) {
      *error = "empty slice entry in '" + spec + "'";
      return false;
    }
    const std::size_t range_pos = raw_part.find("..");
    if (range_pos == std::string::npos) {
      int value = 0;
      if (!ParseInt(raw_part, &value)) {
        *error = "invalid slice '" + raw_part + "'";
        return false;
      }
      if (seen.insert(value).second) {
        parsed.push_back(value);
      }
      continue;
    }

    const std::string begin_text = raw_part.substr(0, range_pos);
    const std::string end_text = raw_part.substr(range_pos + 2);
    int begin = 0;
    int end = 0;
    if (!ParseInt(begin_text, &begin) || !ParseInt(end_text, &end)) {
      *error = "invalid slice range '" + raw_part + "'";
      return false;
    }
    if (begin > end) {
      *error = "slice range begin is greater than end in '" + raw_part + "'";
      return false;
    }
    for (int value = begin; value <= end; ++value) {
      if (seen.insert(value).second) {
        parsed.push_back(value);
      }
    }
  }

  if (parsed.empty()) {
    *error = "slice set is empty";
    return false;
  }
  std::sort(parsed.begin(), parsed.end());
  *slices = std::move(parsed);
  return true;
}

std::string Location(const std::string& source_name, int line_number) {
  return source_name + ":" + std::to_string(line_number) + ": ";
}

}  // namespace

ParseResult ParseDfgFile(const std::string& path) {
  std::ifstream input(path);
  if (!input) {
    return ParseResult{false, Dfg{}, "failed to open DFG file: " + path};
  }
  std::ostringstream buffer;
  buffer << input.rdbuf();
  return ParseDfgText(buffer.str(), path);
}

ParseResult ParseDfgText(const std::string& text, const std::string& source_name) {
  ParseResult result;
  Dfg graph;

  std::istringstream input(text);
  std::string line;
  int line_number = 0;
  while (std::getline(input, line)) {
    ++line_number;
    const std::size_t comment_pos = line.find('#');
    if (comment_pos != std::string::npos) {
      line = line.substr(0, comment_pos);
    }
    line = Trim(line);
    if (line.empty()) {
      continue;
    }

    const std::vector<std::string> tokens = SplitWhitespace(line);
    if (tokens.empty()) {
      continue;
    }

    if (tokens[0] == "op") {
      if (tokens.size() < 4) {
        result.error = Location(source_name, line_number) +
                       "expected: op <id> <kind> <dfunc> [slices=<spec>]";
        return result;
      }
      if (graph.FindOperationIndex(tokens[1]) >= 0) {
        result.error = Location(source_name, line_number) +
                       "duplicate operation id '" + tokens[1] + "'";
        return result;
      }

      OpKind kind = OpKind::kCompute;
      if (!ParseOpKind(tokens[2], &kind)) {
        result.error = Location(source_name, line_number) +
                       "unknown operation kind '" + tokens[2] + "'";
        return result;
      }

      int dfunc = 0;
      if (!ParseInt(tokens[3], &dfunc) || dfunc < 0) {
        result.error = Location(source_name, line_number) +
                       "dfunc must be a non-negative integer";
        return result;
      }

      Operation op;
      op.id = tokens[1];
      op.kind = kind;
      op.dfunc = dfunc;

      for (std::size_t i = 4; i < tokens.size(); ++i) {
        const std::string prefix = "slices=";
        if (tokens[i].rfind(prefix, 0) != 0) {
          result.error = Location(source_name, line_number) +
                         "unknown op attribute '" + tokens[i] + "'";
          return result;
        }
        std::vector<int> slices;
        std::string error;
        if (!ParseSliceSpec(tokens[i].substr(prefix.size()), &slices, &error)) {
          result.error = Location(source_name, line_number) + error;
          return result;
        }
        op.feasible_slices = std::move(slices);
      }

      graph.AddOperation(std::move(op));
      continue;
    }

    if (tokens[0] == "edge") {
      if (tokens.size() != 3) {
        result.error =
            Location(source_name, line_number) + "expected: edge <src> <dst>";
        return result;
      }
      const int src = graph.FindOperationIndex(tokens[1]);
      const int dst = graph.FindOperationIndex(tokens[2]);
      if (src < 0 || dst < 0) {
        result.error = Location(source_name, line_number) +
                       "edge references an unknown operation";
        return result;
      }
      graph.AddEdgeByIndex(src, dst);
      continue;
    }

    result.error = Location(source_name, line_number) +
                   "unknown record type '" + tokens[0] + "'";
    return result;
  }

  result.ok = true;
  result.graph = std::move(graph);
  return result;
}

}  // namespace dfg_ilp
