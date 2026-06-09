#include "model/ilp_solver.h"

#include <chrono>
#include <cmath>
#include <memory>
#include <optional>
#include <stdexcept>

#include "dfg/validator.h"
#include "model/constraints/legal_slice.h"
#include "model/constraints/slice_icu.h"
#include "model/constraints/stream.h"
#include "model/constraints/timing.h"
#include "model/objective.h"
#include "model/preprocess.h"
#include "model/variables.h"
#include "model/solution_checker.h"
#include "ortools/linear_solver/linear_solver.h"

namespace dfg_ilp {
namespace {

using operations_research::MPVariable;
using operations_research::MPSolver;
using Clock = std::chrono::steady_clock;

long long ElapsedMs(Clock::time_point begin, Clock::time_point end) {
  return std::chrono::duration_cast<std::chrono::milliseconds>(end - begin)
      .count();
}

int RoundedValue(const MPVariable* variable) {
  return static_cast<int>(std::llround(variable->solution_value()));
}

SolveStatus MapStatus(MPSolver::ResultStatus status) {
  switch (status) {
    case MPSolver::OPTIMAL:
      return SolveStatus::kOptimal;
    case MPSolver::FEASIBLE:
      return SolveStatus::kFeasible;
    case MPSolver::INFEASIBLE:
      return SolveStatus::kInfeasible;
    case MPSolver::UNBOUNDED:
      return SolveStatus::kUnbounded;
    case MPSolver::ABNORMAL:
    case MPSolver::MODEL_INVALID:
      return SolveStatus::kAbnormal;
    case MPSolver::NOT_SOLVED:
      return SolveStatus::kNotSolved;
  }
  return SolveStatus::kAbnormal;
}

int SelectedStream(const VariableContext& vars, int edge_index) {
  int best_stream = -1;
  double best_value = -1.0;
  for (int s = 0; s < static_cast<int>(vars.locate[edge_index].size()); ++s) {
    const double value = vars.locate[edge_index][s]->solution_value();
    if (value > best_value) {
      best_value = value;
      best_stream = s;
    }
  }
  return best_stream;
}

void FillSolution(const Dfg& graph, const VariableContext& vars,
                  const MPSolver& solver, SolveResult* result) {
  result->objective_value = solver.Objective().Value();
  result->makespan = RoundedValue(vars.makespan);
  result->variable_count = solver.NumVariables();
  result->constraint_count = solver.NumConstraints();

  result->operations.reserve(graph.NumOps());
  for (int i = 0; i < graph.NumOps(); ++i) {
    const Operation& op = graph.operation(i);
    const int issue = RoundedValue(vars.issue[i]);
    result->operations.push_back(OpAssignment{
        op.id,
        ToString(op.kind),
        op.dfunc,
        issue,
        issue + op.dfunc,
        RoundedValue(vars.pos[i]),
    });
  }

  result->edges.reserve(graph.NumEdges());
  for (int e = 0; e < graph.NumEdges(); ++e) {
    const Edge& edge = graph.edge(e);
    result->edges.push_back(EdgeAssignment{
        graph.operation(edge.src).id,
        graph.operation(edge.dst).id,
        SelectedStream(vars, e),
        vars.east[e]->solution_value() >= 0.5 ? "east" : "west",
        RoundedValue(vars.lo[e]),
        RoundedValue(vars.hi[e]),
        RoundedValue(vars.phase[e]),
    });
  }
}

void FillBaselineMetrics(const Dfg& graph, const Hardware& hw,
                         SolveResult* result) {
  const int n = graph.NumOps();
  result->total_op_pairs = n * (n - 1) / 2;
  result->active_op_pairs = result->total_op_pairs;
  result->pruned_op_pairs = 0;
  result->pair_reduction_rate = 0.0;
  result->average_issue_window = hw.max_issue_time;
}

void FillPreprocessMetrics(const GraphPreprocessResult& preprocess,
                           SolveResult* result) {
  result->total_op_pairs = preprocess.total_op_pairs;
  result->active_op_pairs = preprocess.active_op_pairs;
  result->pruned_op_pairs = preprocess.pruned_op_pairs;
  result->pair_reduction_rate = preprocess.pair_reduction_rate;
  result->average_issue_window = preprocess.average_issue_window;
}

}  // namespace

IlpSolver::IlpSolver(Hardware hardware, SolverConfig config)
    : hardware_(std::move(hardware)), config_(std::move(config)) {}

SolveResult IlpSolver::Solve(const Dfg& graph) const {
  const ValidationResult validation = ValidateDfg(graph);
  if (!validation.ok) {
    throw std::runtime_error("invalid DFG: " + validation.errors.front());
  }

  std::unique_ptr<MPSolver> solver(MPSolver::CreateSolver(config_.backend));
  if (!solver) {
    throw std::runtime_error("OR-Tools backend is unavailable: " + config_.backend);
  }
  if (config_.time_limit_ms > 0) {
    solver->set_time_limit(config_.time_limit_ms);
  }

  SolveResult result;
  result.mode = config_.mode;

  std::optional<GraphPreprocessResult> preprocess;
  if (config_.mode == SolverMode::kGraphPreprocessed) {
    const Clock::time_point begin = Clock::now();
    preprocess = PreprocessGraph(graph, hardware_);
    result.preprocess_time_ms = ElapsedMs(begin, Clock::now());
    FillPreprocessMetrics(*preprocess, &result);
  } else {
    FillBaselineMetrics(graph, hardware_, &result);
  }

  const std::vector<OpPairKey> all_pairs = AllOperationPairs(graph);
  const std::vector<OpPairKey> edge_pairs = EdgeEndpointPairs(graph);
  const std::vector<OpPairKey> resource_pairs =
      preprocess ? SafeGraphResourcePairs(graph, preprocess->incomparable_pairs)
                 : all_pairs;
  const std::vector<OpPairKey> distance_pairs =
      MergeOperationPairs(edge_pairs, resource_pairs);
  result.active_op_pairs = static_cast<int>(resource_pairs.size());
  result.pruned_op_pairs = result.total_op_pairs - result.active_op_pairs;
  result.pair_reduction_rate =
      result.total_op_pairs > 0
          ? static_cast<double>(result.pruned_op_pairs) /
                static_cast<double>(result.total_op_pairs)
          : 0.0;

  VariableContext vars(solver.get());
  CreateCoreVariables(graph, hardware_, &vars,
                      preprocess ? &preprocess->asap_issue : nullptr,
                      preprocess ? &preprocess->alap_issue : nullptr);
  CreateSliceIcuVariables(distance_pairs, resource_pairs, &vars);
  CreateStreamVariables(graph, hardware_, &vars);

  AddLegalSliceConstraints(graph, hardware_, &vars);
  AddSliceIcuConstraints(graph, hardware_, distance_pairs, resource_pairs, &vars);
  AddTimingConstraints(graph, hardware_, &vars);
  AddStreamConstraints(graph, hardware_, &vars);
  AddMakespanObjective(graph, &vars);

  const Clock::time_point solve_begin = Clock::now();
  const MPSolver::ResultStatus status = solver->Solve();
  result.solve_time_ms = ElapsedMs(solve_begin, Clock::now());
  result.status = MapStatus(status);
  result.variable_count = solver->NumVariables();
  result.constraint_count = solver->NumConstraints();

  if (status == MPSolver::OPTIMAL || status == MPSolver::FEASIBLE) {
    if (config_.verify_solution && !solver->VerifySolution(1e-6, true)) {
      throw std::runtime_error("OR-Tools reported a solution that failed verification");
    }
    FillSolution(graph, vars, *solver, &result);
    const DomainCheckResult domain_check =
        CheckSolutionDomain(graph, hardware_, result);
    result.domain_check_ok = domain_check.ok;
    result.domain_check_errors = domain_check.errors;
  }

  return result;
}

}  // namespace dfg_ilp
