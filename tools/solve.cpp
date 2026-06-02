#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include "dfg/parser.h"
#include "dfg/validator.h"
#include "model/hardware.h"
#include "model/ilp_solver.h"
#include "visualization/exporter.h"

namespace {

void PrintUsage(const char* argv0) {
  std::cerr << "usage: " << argv0
            << " <input.dfg> <output.json> [--time_limit_ms N] [--solver NAME]\n";
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

}  // namespace

int main(int argc, char** argv) {
  if (argc < 3) {
    PrintUsage(argv[0]);
    return EXIT_FAILURE;
  }

  const std::string input_path = argv[1];
  const std::string output_path = argv[2];

  dfg_ilp::SolverConfig config;
  for (int i = 3; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--time_limit_ms") {
      if (i + 1 >= argc || !ParseInt(argv[++i], &config.time_limit_ms)) {
        std::cerr << "--time_limit_ms expects an integer\n";
        return EXIT_FAILURE;
      }
    } else if (arg == "--solver") {
      if (i + 1 >= argc) {
        std::cerr << "--solver expects a backend name\n";
        return EXIT_FAILURE;
      }
      config.backend = argv[++i];
    } else if (arg == "--no_verify") {
      config.verify_solution = false;
    } else {
      std::cerr << "unknown argument: " << arg << '\n';
      PrintUsage(argv[0]);
      return EXIT_FAILURE;
    }
  }

  const dfg_ilp::ParseResult parse = dfg_ilp::ParseDfgFile(input_path);
  if (!parse.ok) {
    std::cerr << parse.error << '\n';
    return EXIT_FAILURE;
  }

  const dfg_ilp::ValidationResult validation = dfg_ilp::ValidateDfg(parse.graph);
  if (!validation.ok) {
    for (const std::string& error : validation.errors) {
      std::cerr << error << '\n';
    }
    return EXIT_FAILURE;
  }

  dfg_ilp::Hardware hardware;
  dfg_ilp::IlpSolver solver(hardware, config);

  try {
    const dfg_ilp::SolveResult result = solver.Solve(parse.graph);
    std::string error;
    if (!dfg_ilp::WriteSolutionJson(result, output_path, &error)) {
      std::cerr << error << '\n';
      return EXIT_FAILURE;
    }

    std::cout << "status=" << dfg_ilp::ToString(result.status)
              << " makespan=" << result.makespan
              << " vars=" << result.variable_count
              << " constraints=" << result.constraint_count << '\n';
    if (result.status != dfg_ilp::SolveStatus::kOptimal &&
        result.status != dfg_ilp::SolveStatus::kFeasible) {
      return EXIT_FAILURE;
    }
  } catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
