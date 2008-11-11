/*-------------------------------------------------------------------------
 *
 * nodeSkyline.h
 *	  prototypes for nodeSkyline.c
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
#ifndef NODESKYLINE_H
#define NODESKYLINE_H

#include "nodes/execnodes.h"

extern int	ExecCountSlotsSkyline(Skyline *node);
extern SkylineState *ExecInitSkyline(Skyline *node, EState *estate, int eflags);
extern TupleTableSlot *ExecSkyline(SkylineState *node);
extern void ExecEndSkyline(SkylineState *node);
extern void ExecReScanSkyline(SkylineState *node, ExprContext *exprCtxt);


#endif   /* NODESKYLINE_H */
