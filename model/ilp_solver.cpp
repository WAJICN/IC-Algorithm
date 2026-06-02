#include "model/ilp_solver.h"

#include <cmath>
#include <memory>
#include <stdexcept>

#include "dfg/validator.h"
#include "model/constraints/legal_slice.h"
#include "model/constraints/slice_icu.h"
#include "model/constraints/stream.h"
#include "model/constraints/timing.h"
#include "model/objective.h"
#include "model/variables.h"
#include "ortools/linear_solver/linear_solver.h"

namespace dfg_ilp {
namespace {

using operations_research::MPVariable;
using operations_research::MPSolver;

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

  VariableContext vars(solver.get());
  CreateCoreVariables(graph, hardware_, &vars);
  CreateSliceIcuVariables(graph, &vars);
  CreateStreamVariables(graph, hardware_, &vars);

  AddLegalSliceConstraints(graph, hardware_, &vars);
  AddSliceIcuConstraints(graph, hardware_, &vars);
  AddTimingConstraints(graph, hardware_, &vars);
  AddStreamConstraints(graph, hardware_, &vars);
  AddMakespanObjective(graph, &vars);

  SolveResult result;
  const MPSolver::ResultStatus status = solver->Solve();
  result.status = MapStatus(status);
  result.variable_count = solver->NumVariables();
  result.constraint_count = solver->NumConstraints();

  if (status == MPSolver::OPTIMAL || status == MPSolver::FEASIBLE) {
    if (config_.verify_solution && !solver->VerifySolution(1e-6, true)) {
      throw std::runtime_error("OR-Tools reported a solution that failed verification");
    }
    FillSolution(graph, vars, *solver, &result);
  }

  return result;
}

}  // namespace dfg_ilp
