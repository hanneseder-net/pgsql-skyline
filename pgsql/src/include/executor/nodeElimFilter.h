#ifndef NODEELIMFILTER_H
#define NODEELIMFILTER_H

#include "nodes/execnodes.h"

extern int	ExecCountSlotsElimFilter(ElimFilter *node);
extern ElimFilterState *ExecInitElimFilter(ElimFilter *node, EState *estate, int eflags);
extern TupleTableSlot *ExecElimFilter(ElimFilterState *node);
extern void ExecEndElimFilter(ElimFilterState *node);
extern void ExecReScanElimFilter(ElimFilterState *node, ExprContext *exprCtxt);


#endif   /* NODEELIMFILTER_H */
