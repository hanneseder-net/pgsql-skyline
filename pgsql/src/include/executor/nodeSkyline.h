#ifndef NODESKYLINE_H
#define NODESKYLINE_H

#include "nodes/execnodes.h"

extern int	ExecCountSlotsSkyline(Skyline *node);
extern SkylineState *ExecInitSkyline(Skyline *node, EState *estate, int eflags);
extern TupleTableSlot *ExecSkyline(SkylineState *node);
extern void ExecEndSkyline(SkylineState *node);
/*
extern void ExecReScanGroup(GroupState *node, ExprContext *exprCtxt);
*/

#endif   /* NODESKYLINE_H */