#ifndef MODEL_OBJECTIVE_H_
#define MODEL_OBJECTIVE_H_

#include "dfg/dfg.h"
#include "model/variables.h"

namespace dfg_ilp {

void AddMakespanObjective(const Dfg& graph, VariableContext* vars);

}  // namespace dfg_ilp

#endif  // MODEL_OBJECTIVE_H_
