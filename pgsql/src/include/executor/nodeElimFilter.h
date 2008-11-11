/*-------------------------------------------------------------------------
 *
 * nodeElimFilter.c
 *	  prototypes for nodeElimFilter.c
 *
 * Portions Copyright (c) 2008, PostgreSQL Global Development Group
 * Portions Copyright (c) 2007-2008, Hannes Eder
 *
 *
 * DESCRIPTION
 *	  FIXME
 *
 * IDENTIFICATION
 *	  $PostgreSQL: $
 *
 *-------------------------------------------------------------------------
 */
#ifndef NODEELIMFILTER_H
#define NODEELIMFILTER_H

#include "nodes/execnodes.h"

extern int	ExecCountSlotsElimFilter(ElimFilter *node);
extern ElimFilterState *ExecInitElimFilter(ElimFilter *node, EState *estate, int eflags);
extern TupleTableSlot *ExecElimFilter(ElimFilterState *node);
extern void ExecEndElimFilter(ElimFilterState *node);
extern void ExecReScanElimFilter(ElimFilterState *node, ExprContext *exprCtxt);


#endif   /* NODEELIMFILTER_H */
