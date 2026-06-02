#ifndef MODEL_CONSTRAINTS_SLICE_ICU_H_
#define MODEL_CONSTRAINTS_SLICE_ICU_H_

#include "dfg/dfg.h"
#include "model/hardware.h"
#include "model/variables.h"

namespace dfg_ilp {

void AddSliceIcuConstraints(const Dfg& graph, const Hardware& hw,
                            VariableContext* vars);

}  // namespace dfg_ilp

#endif  // MODEL_CONSTRAINTS_SLICE_ICU_H_
