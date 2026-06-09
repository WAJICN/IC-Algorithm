#include "visualization/exporter.h"

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace dfg_ilp {
namespace {

std::string JsonEscape(const std::string& text) {
  std::ostringstream out;
  for (const char c : text) {
    switch (c) {
      case '\\':
        out << "\\\\";
        break;
      case '"':
        out << "\\\"";
        break;
      case '\n':
        out << "\\n";
        break;
      case '\r':
        out << "\\r";
        break;
      case '\t':
        out << "\\t";
        break;
      default:
        out << c;
        break;
    }
  }
  return out.str();
}

bool EnsureParentDirectory(const std::string& path, std::string* error) {
  const std::filesystem::path fs_path(path);
  const std::filesystem::path parent = fs_path.parent_path();
  if (parent.empty()) {
    return true;
  }
  std::error_code ec;
  std::filesystem::create_directories(parent, ec);
  if (ec) {
    *error = "failed to create output directory '" + parent.string() +
             "': " + ec.message();
    return false;
  }
  return true;
}

}  // namespace

bool WriteSolutionJson(const SolveResult& result, const std::string& path,
                       std::string* error) {
  if (!EnsureParentDirectory(path, error)) {
    return false;
  }
  std::ofstream out(path);
  if (!out) {
    *error = "failed to open output file: " + path;
    return false;
  }

  out << "{\n";
  out << "  \"mode\": \"" << JsonEscape(ToString(result.mode)) << "\",\n";
  out << "  \"status\": \"" << JsonEscape(ToString(result.status)) << "\",\n";
  out << "  \"objective_value\": " << std::fixed << std::setprecision(6)
      << result.objective_value << ",\n";
  out << "  \"makespan\": " << result.makespan << ",\n";
  out << "  \"variable_count\": " << result.variable_count << ",\n";
  out << "  \"constraint_count\": " << result.constraint_count << ",\n";
  out << "  \"solve_time_ms\": " << result.solve_time_ms << ",\n";
  out << "  \"preprocess_time_ms\": " << result.preprocess_time_ms << ",\n";
  out << "  \"preprocess\": {\n";
  out << "    \"total_op_pairs\": " << result.total_op_pairs << ",\n";
  out << "    \"active_op_pairs\": " << result.active_op_pairs << ",\n";
  out << "    \"pruned_op_pairs\": " << result.pruned_op_pairs << ",\n";
  out << "    \"pair_reduction_rate\": " << std::fixed << std::setprecision(6)
      << result.pair_reduction_rate << ",\n";
  out << "    \"average_issue_window\": " << std::fixed << std::setprecision(6)
      << result.average_issue_window << "\n";
  out << "  },\n";
  out << "  \"domain_check\": {\n";
  out << "    \"ok\": " << (result.domain_check_ok ? "true" : "false") << ",\n";
  out << "    \"error_count\": " << result.domain_check_errors.size() << ",\n";
  out << "    \"errors\": [\n";
  for (std::size_t i = 0; i < result.domain_check_errors.size(); ++i) {
    out << "      \"" << JsonEscape(result.domain_check_errors[i]) << "\"";
    out << (i + 1 == result.domain_check_errors.size() ? "\n" : ",\n");
  }
  out << "    ]\n";
  out << "  },\n";
  out << "  \"operations\": [\n";
  for (std::size_t i = 0; i < result.operations.size(); ++i) {
    const OpAssignment& op = result.operations[i];
    out << "    {\"id\": \"" << JsonEscape(op.id) << "\", "
        << "\"kind\": \"" << JsonEscape(op.kind) << "\", "
        << "\"dfunc\": " << op.dfunc << ", "
        << "\"issue\": " << op.issue << ", "
        << "\"finish\": " << op.finish << ", "
        << "\"pos\": " << op.pos << "}";
    out << (i + 1 == result.operations.size() ? "\n" : ",\n");
  }
  out << "  ],\n";
  out << "  \"edges\": [\n";
  for (std::size_t i = 0; i < result.edges.size(); ++i) {
    const EdgeAssignment& edge = result.edges[i];
    out << "    {\"src\": \"" << JsonEscape(edge.src) << "\", "
        << "\"dst\": \"" << JsonEscape(edge.dst) << "\", "
        << "\"stream\": " << edge.stream << ", "
        << "\"direction\": \"" << JsonEscape(edge.direction) << "\", "
        << "\"lo\": " << edge.lo << ", "
        << "\"hi\": " << edge.hi << ", "
        << "\"phase\": " << edge.phase << "}";
    out << (i + 1 == result.edges.size() ? "\n" : ",\n");
  }
  out << "  ]\n";
  out << "}\n";
  return true;
}

bool WriteSolutionCsv(const SolveResult& result, const std::string& path,
                      std::string* error) {
  if (!EnsureParentDirectory(path, error)) {
    return false;
  }
  std::ofstream out(path);
  if (!out) {
    *error = "failed to open output file: " + path;
    return false;
  }

  out << "id,kind,dfunc,issue,finish,pos\n";
  for (const OpAssignment& op : result.operations) {
    out << op.id << ',' << op.kind << ',' << op.dfunc << ',' << op.issue << ','
        << op.finish << ',' << op.pos << '\n';
  }
  return true;
}

}  // namespace dfg_ilp
