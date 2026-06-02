#include "model/constraints/stream.h"

#include "ortools/linear_solver/linear_solver.h"

namespace dfg_ilp {
namespace {

using operations_research::MPConstraint;
using operations_research::MPVariable;

void AddAndConstraints(VariableContext* vars, MPVariable* out, MPVariable* a,
                       MPVariable* b, const std::string& prefix, int i, int j) {
  MPConstraint* c1 =
      vars->solver->MakeRowConstraint(-vars->solver->infinity(), 0.0,
                                      VarName(prefix + "_le_a", i, j));
  c1->SetCoefficient(out, 1.0);
  c1->SetCoefficient(a, -1.0);

  MPConstraint* c2 =
      vars->solver->MakeRowConstraint(-vars->solver->infinity(), 0.0,
                                      VarName(prefix + "_le_b", i, j));
  c2->SetCoefficient(out, 1.0);
  c2->SetCoefficient(b, -1.0);

  MPConstraint* c3 =
      vars->solver->MakeRowConstraint(-1.0, vars->solver->infinity(),
                                      VarName(prefix + "_ge_ab", i, j));
  c3->SetCoefficient(out, 1.0);
  c3->SetCoefficient(a, -1.0);
  c3->SetCoefficient(b, -1.0);
}

void AddSameStreamAnd(VariableContext* vars, MPVariable* out, MPVariable* a,
                      MPVariable* b, int e1, int e2, int stream) {
  MPConstraint* c1 = vars->solver->MakeRowConstraint(
      -vars->solver->infinity(), 0.0,
      VarName("same_stream_le_a", e1, e2, stream));
  c1->SetCoefficient(out, 1.0);
  c1->SetCoefficient(a, -1.0);

  MPConstraint* c2 = vars->solver->MakeRowConstraint(
      -vars->solver->infinity(), 0.0,
      VarName("same_stream_le_b", e1, e2, stream));
  c2->SetCoefficient(out, 1.0);
  c2->SetCoefficient(b, -1.0);

  MPConstraint* c3 = vars->solver->MakeRowConstraint(
      -1.0, vars->solver->infinity(),
      VarName("same_stream_ge_ab", e1, e2, stream));
  c3->SetCoefficient(out, 1.0);
  c3->SetCoefficient(a, -1.0);
  c3->SetCoefficient(b, -1.0);
}

void AddDirectionConstraints(const Dfg& graph, const Hardware& hw,
                             VariableContext* vars, int e) {
  const Edge& edge = graph.edge(e);

  MPConstraint* east_guard = vars->solver->MakeRowConstraint(
      -vars->solver->infinity(), 0.0, VarName("east_guard", e));
  east_guard->SetCoefficient(vars->pos[edge.dst], 1.0);
  east_guard->SetCoefficient(vars->pos[edge.src], -1.0);
  east_guard->SetCoefficient(vars->east[e], -hw.big_m);

  MPConstraint* west_guard = vars->solver->MakeRowConstraint(
      -vars->solver->infinity(), 0.0, VarName("west_guard", e));
  west_guard->SetCoefficient(vars->pos[edge.src], 1.0);
  west_guard->SetCoefficient(vars->pos[edge.dst], -1.0);
  west_guard->SetCoefficient(vars->west[e], -hw.big_m);

  MPConstraint* one_dir =
      vars->solver->MakeRowConstraint(1.0, 1.0, VarName("one_dir", e));
  one_dir->SetCoefficient(vars->east[e], 1.0);
  one_dir->SetCoefficient(vars->west[e], 1.0);
}

void AddStreamChoiceConstraints(const Hardware& hw, VariableContext* vars, int e) {
  MPConstraint* one_stream =
      vars->solver->MakeRowConstraint(1.0, 1.0, VarName("one_stream", e));
  for (int s = 0; s < hw.stream_count; ++s) {
    one_stream->SetCoefficient(vars->locate[e][s], 1.0);
  }
}

void AddRouteIntervalConstraints(const Dfg& graph, const Hardware& hw,
                                 VariableContext* vars, int e) {
  const Edge& edge = graph.edge(e);

  MPConstraint* lo_src_east = vars->solver->MakeRowConstraint(
      -vars->solver->infinity(), hw.big_m, VarName("lo_src_east_a", e));
  lo_src_east->SetCoefficient(vars->lo[e], 1.0);
  lo_src_east->SetCoefficient(vars->pos[edge.src], -1.0);
  lo_src_east->SetCoefficient(vars->east[e], hw.big_m);

  MPConstraint* src_lo_east = vars->solver->MakeRowConstraint(
      -vars->solver->infinity(), hw.big_m, VarName("lo_src_east_b", e));
  src_lo_east->SetCoefficient(vars->pos[edge.src], 1.0);
  src_lo_east->SetCoefficient(vars->lo[e], -1.0);
  src_lo_east->SetCoefficient(vars->east[e], hw.big_m);

  MPConstraint* hi_dst_east = vars->solver->MakeRowConstraint(
      -vars->solver->infinity(), hw.big_m, VarName("hi_dst_east_a", e));
  hi_dst_east->SetCoefficient(vars->hi[e], 1.0);
  hi_dst_east->SetCoefficient(vars->pos[edge.dst], -1.0);
  hi_dst_east->SetCoefficient(vars->east[e], hw.big_m);

  MPConstraint* dst_hi_east = vars->solver->MakeRowConstraint(
      -vars->solver->infinity(), hw.big_m, VarName("hi_dst_east_b", e));
  dst_hi_east->SetCoefficient(vars->pos[edge.dst], 1.0);
  dst_hi_east->SetCoefficient(vars->hi[e], -1.0);
  dst_hi_east->SetCoefficient(vars->east[e], hw.big_m);

  MPConstraint* lo_dst_west = vars->solver->MakeRowConstraint(
      -vars->solver->infinity(), hw.big_m, VarName("lo_dst_west_a", e));
  lo_dst_west->SetCoefficient(vars->lo[e], 1.0);
  lo_dst_west->SetCoefficient(vars->pos[edge.dst], -1.0);
  lo_dst_west->SetCoefficient(vars->west[e], hw.big_m);

  MPConstraint* dst_lo_west = vars->solver->MakeRowConstraint(
      -vars->solver->infinity(), hw.big_m, VarName("lo_dst_west_b", e));
  dst_lo_west->SetCoefficient(vars->pos[edge.dst], 1.0);
  dst_lo_west->SetCoefficient(vars->lo[e], -1.0);
  dst_lo_west->SetCoefficient(vars->west[e], hw.big_m);

  MPConstraint* hi_src_west = vars->solver->MakeRowConstraint(
      -vars->solver->infinity(), hw.big_m, VarName("hi_src_west_a", e));
  hi_src_west->SetCoefficient(vars->hi[e], 1.0);
  hi_src_west->SetCoefficient(vars->pos[edge.src], -1.0);
  hi_src_west->SetCoefficient(vars->west[e], hw.big_m);

  MPConstraint* src_hi_west = vars->solver->MakeRowConstraint(
      -vars->solver->infinity(), hw.big_m, VarName("hi_src_west_b", e));
  src_hi_west->SetCoefficient(vars->pos[edge.src], 1.0);
  src_hi_west->SetCoefficient(vars->hi[e], -1.0);
  src_hi_west->SetCoefficient(vars->west[e], hw.big_m);
}

void AddPhaseConstraints(const Dfg& graph, const Hardware& hw,
                         VariableContext* vars, int e) {
  const Edge& edge = graph.edge(e);
  const int dfunc = graph.operation(edge.src).dfunc;

  MPConstraint* phase_east_a = vars->solver->MakeRowConstraint(
      -vars->solver->infinity(), hw.big_m + dfunc,
      VarName("phase_east_a", e));
  phase_east_a->SetCoefficient(vars->phase[e], 1.0);
  phase_east_a->SetCoefficient(vars->issue[edge.src], -1.0);
  phase_east_a->SetCoefficient(vars->pos[edge.src], 1.0);
  phase_east_a->SetCoefficient(vars->east[e], hw.big_m);

  MPConstraint* phase_east_b = vars->solver->MakeRowConstraint(
      -vars->solver->infinity(), hw.big_m - dfunc,
      VarName("phase_east_b", e));
  phase_east_b->SetCoefficient(vars->issue[edge.src], 1.0);
  phase_east_b->SetCoefficient(vars->pos[edge.src], -1.0);
  phase_east_b->SetCoefficient(vars->phase[e], -1.0);
  phase_east_b->SetCoefficient(vars->east[e], hw.big_m);

  MPConstraint* phase_west_a = vars->solver->MakeRowConstraint(
      -vars->solver->infinity(), hw.big_m + dfunc,
      VarName("phase_west_a", e));
  phase_west_a->SetCoefficient(vars->phase[e], 1.0);
  phase_west_a->SetCoefficient(vars->issue[edge.src], -1.0);
  phase_west_a->SetCoefficient(vars->pos[edge.src], -1.0);
  phase_west_a->SetCoefficient(vars->west[e], hw.big_m);

  MPConstraint* phase_west_b = vars->solver->MakeRowConstraint(
      -vars->solver->infinity(), hw.big_m - dfunc,
      VarName("phase_west_b", e));
  phase_west_b->SetCoefficient(vars->issue[edge.src], 1.0);
  phase_west_b->SetCoefficient(vars->pos[edge.src], 1.0);
  phase_west_b->SetCoefficient(vars->phase[e], -1.0);
  phase_west_b->SetCoefficient(vars->west[e], hw.big_m);
}

void AddPairwiseRouteConstraints(const Hardware& hw, VariableContext* vars, int e1,
                                 int e2) {
  const EdgePairKey key{e1, e2};
  AddAndConstraints(vars, vars->same_east.at(key), vars->east[e1], vars->east[e2],
                    "same_east", e1, e2);
  AddAndConstraints(vars, vars->same_west.at(key), vars->west[e1], vars->west[e2],
                    "same_west", e1, e2);

  MPConstraint* same_dir =
      vars->solver->MakeRowConstraint(0.0, 0.0, VarName("same_dir_def", e1, e2));
  same_dir->SetCoefficient(vars->same_stream_dir.at(key), 1.0);
  same_dir->SetCoefficient(vars->same_east.at(key), -1.0);
  same_dir->SetCoefficient(vars->same_west.at(key), -1.0);

  for (int s = 0; s < hw.stream_count; ++s) {
    AddSameStreamAnd(vars, vars->same_stream.at({e1, e2, s}),
                     vars->locate[e1][s], vars->locate[e2][s], e1, e2, s);
  }

  MPConstraint* same_stream_id =
      vars->solver->MakeRowConstraint(0.0, 0.0,
                                      VarName("same_stream_id_def", e1, e2));
  same_stream_id->SetCoefficient(vars->same_stream_id.at(key), 1.0);
  for (int s = 0; s < hw.stream_count; ++s) {
    same_stream_id->SetCoefficient(vars->same_stream.at({e1, e2, s}), -1.0);
  }

  MPConstraint* before_i_a = vars->solver->MakeRowConstraint(
      -vars->solver->infinity(), hw.big_m - 1, VarName("before_i_a", e1, e2));
  before_i_a->SetCoefficient(vars->hi[e1], 1.0);
  before_i_a->SetCoefficient(vars->lo[e2], -1.0);
  before_i_a->SetCoefficient(vars->before_i.at(key), hw.big_m);

  MPConstraint* before_i_b = vars->solver->MakeRowConstraint(
      -vars->solver->infinity(), 0.0, VarName("before_i_b", e1, e2));
  before_i_b->SetCoefficient(vars->hi[e1], -1.0);
  before_i_b->SetCoefficient(vars->lo[e2], 1.0);
  before_i_b->SetCoefficient(vars->before_i.at(key), -hw.big_m);

  MPConstraint* before_ii_a = vars->solver->MakeRowConstraint(
      -vars->solver->infinity(), hw.big_m - 1, VarName("before_ii_a", e1, e2));
  before_ii_a->SetCoefficient(vars->hi[e2], 1.0);
  before_ii_a->SetCoefficient(vars->lo[e1], -1.0);
  before_ii_a->SetCoefficient(vars->before_ii.at(key), hw.big_m);

  MPConstraint* before_ii_b = vars->solver->MakeRowConstraint(
      -vars->solver->infinity(), 0.0, VarName("before_ii_b", e1, e2));
  before_ii_b->SetCoefficient(vars->hi[e2], -1.0);
  before_ii_b->SetCoefficient(vars->lo[e1], 1.0);
  before_ii_b->SetCoefficient(vars->before_ii.at(key), -hw.big_m);

  MPConstraint* overlap_def =
      vars->solver->MakeRowConstraint(1.0, 1.0, VarName("overlap_def", e1, e2));
  overlap_def->SetCoefficient(vars->before_i.at(key), 1.0);
  overlap_def->SetCoefficient(vars->before_ii.at(key), 1.0);
  overlap_def->SetCoefficient(vars->overlap.at(key), 1.0);

  MPConstraint* phase_order_a = vars->solver->MakeRowConstraint(
      1.0 - 4 * hw.big_m, vars->solver->infinity(),
      VarName("phase_order_a", e1, e2));
  phase_order_a->SetCoefficient(vars->phase[e1], 1.0);
  phase_order_a->SetCoefficient(vars->phase[e2], -1.0);
  phase_order_a->SetCoefficient(vars->order_delta.at(key), -hw.big_m);
  phase_order_a->SetCoefficient(vars->same_stream_id.at(key), -hw.big_m);
  phase_order_a->SetCoefficient(vars->same_stream_dir.at(key), -hw.big_m);
  phase_order_a->SetCoefficient(vars->overlap.at(key), -hw.big_m);

  MPConstraint* phase_order_b = vars->solver->MakeRowConstraint(
      1.0 - 3 * hw.big_m, vars->solver->infinity(),
      VarName("phase_order_b", e1, e2));
  phase_order_b->SetCoefficient(vars->phase[e2], 1.0);
  phase_order_b->SetCoefficient(vars->phase[e1], -1.0);
  phase_order_b->SetCoefficient(vars->order_delta.at(key), hw.big_m);
  phase_order_b->SetCoefficient(vars->same_stream_id.at(key), -hw.big_m);
  phase_order_b->SetCoefficient(vars->same_stream_dir.at(key), -hw.big_m);
  phase_order_b->SetCoefficient(vars->overlap.at(key), -hw.big_m);
}

}  // namespace

void AddStreamConstraints(const Dfg& graph, const Hardware& hw,
                          VariableContext* vars) {
  for (int e = 0; e < graph.NumEdges(); ++e) {
    AddDirectionConstraints(graph, hw, vars, e);
    AddStreamChoiceConstraints(hw, vars, e);
    AddRouteIntervalConstraints(graph, hw, vars, e);
    AddPhaseConstraints(graph, hw, vars, e);
  }

  for (int e1 = 0; e1 < graph.NumEdges(); ++e1) {
    for (int e2 = e1 + 1; e2 < graph.NumEdges(); ++e2) {
      AddPairwiseRouteConstraints(hw, vars, e1, e2);
    }
  }
}

}  // namespace dfg_ilp
