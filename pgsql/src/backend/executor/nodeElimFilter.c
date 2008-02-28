/*-------------------------------------------------------------------------
 *
 * nodeElimFilter.c
 *	  Routines to handle skyline LESS elimination Filter
 *
 * Portions Copyright (c) 2007, PostgreSQL Global Development Group
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

#include "postgres.h"

#include "executor/executor.h"
#include "executor/nodeElimFilter.h"
#include "utils/tuplewindow.h"
#include "utils/skyline.h"

/*
 * ExecElimFilterInitTupleWindow
 *
 *	FIXME
 */
static void
ExecElimFilterInitTupleWindow(SkylineState *node, Skyline *sl)
{
	int			window_size = BLCKSZ / 1024;	/* allocate a page */
	int			window_slots = -1;
	TupleWindowPolicy	window_policy = TUP_WIN_POLICY_APPEND;

	Assert(node != NULL);

	if (node->window != NULL)
	{
		tuplewindow_end(node->window);
		node->window = NULL;
	}

	/*
	 * Can be overrided by an option, otherwise use entire
	 * work_mem.
	 */
	skyline_option_get_int(sl->skyline_of_options, "efwindow", &window_size) ||
		skyline_option_get_int(sl->skyline_of_options, "efwindowsize", &window_size);

	skyline_option_get_int(sl->skyline_of_options, "efslots", &window_slots) ||
		skyline_option_get_int(sl->skyline_of_options, "efwindowslots", &window_slots);

	skyline_option_get_window_policy(sl->skyline_of_options, "efwindowpolicy", &window_policy);

	if (window_policy == TUP_WIN_POLICY_RANKED)
		node->flags |= SL_FLAGS_RANKED;

	if (window_slots == 0)
	{
		/*
		 * If window_slots == -1, then we constrain the tuple window in
		 * terms of memory.
		 */
		elog(ERROR, "tuple window must have at least one slot");
	}

	node->window = tuplewindow_begin(window_size, window_slots, window_policy);

	node->windowsize = window_size;
	node->windowslots = window_slots;
}

/*
 * ExecInitElimFilter
 *
 *	FIXME
 */
ElimFilterState *
ExecInitElimFilter(ElimFilter *node, EState *estate, int eflags)
{
	ElimFilterState	   *state;

	state = makeNode(ElimFilterState);
	state->ss.ps.plan = (Plan *) node;
	state->ss.ps.state = estate;
	state->status = SS_INIT;
	state->flags = SL_FLAGS_NONE;

#define ELIMFILTER_NSLOTS 2

	/*
	 * tuple table initialization
	 */
	ExecInitScanTupleSlot(estate, &state->ss);
	ExecInitResultTupleSlot(estate, &state->ss.ps);

	/*
	 * initialize child expressions
	 */
	state->ss.ps.targetlist = (List *)
		ExecInitExpr((Expr *) node->plan.targetlist,
					 (PlanState *) state);
	state->ss.ps.qual = (List *)
		ExecInitExpr((Expr *) node->plan.qual,
					 (PlanState *) state);

	/*
	 * initialize child nodes
	 */
	outerPlanState(state) = ExecInitNode(outerPlan(node), estate, eflags);

	/*
	 * Initialize tuple type.  
	 */
	ExecAssignResultTypeFromTL(&state->ss.ps);
	ExecAssignScanTypeFromOuterPlan(&state->ss);
	ExecAssignProjectionInfo(&state->ss.ps, NULL);
	
	ExecSkylineCacheCompareFunctionInfo(state, node);

	ExecElimFilterInitTupleWindow(state, node);

	return state;
}

/*
 * ExecCountSlotsElimFilter
 *
 *	FIXME
 */
int
ExecCountSlotsElimFilter(ElimFilter *node)
{
	return ExecCountSlotsNode(outerPlan(node)) + ELIMFILTER_NSLOTS;
}

/*
 * ExecEndElimFilter
 *
 *	FIXME
 */
void
ExecEndElimFilter(ElimFilterState *node)
{
	/*
	 * clean out the tuple table
	 */
	ExecClearTuple(node->ss.ss_ScanTupleSlot);
	ExecClearTuple(node->ss.ps.ps_ResultTupleSlot);

	/*
	 * shut down the subplan
	 */
	ExecEndNode(outerPlanState(node));
}

/*
 * ExecReScanElimFilter
 *
 *	FIXME
 */
void
ExecReScanElimFilter(ElimFilterState *node, ExprContext *exprCtxt)
{
	/* FIXME: code coverage = 0 !!! */

	node->status = SS_INIT;

	/* must clear first tuple */
	ExecClearTuple(node->ss.ss_ScanTupleSlot);

	if (((PlanState *) node)->lefttree &&
		((PlanState *) node)->lefttree->chgParam == NULL)
		ExecReScan(((PlanState *) node)->lefttree, exprCtxt);

	/* reinit tuple window */
	ExecElimFilterInitTupleWindow(node, (Skyline *) node->ss.ps.plan);

	/* FIXME: there is maybe more that need to be done here */
}

/*
 * ExecElimFilter
 *
 *	FIXME
 */
TupleTableSlot *
ExecElimFilter(ElimFilterState *node)
{
	TupleWindowState   *window = node->window;
	TupleTableSlot	   *inner_slot = node->ss.ps.ps_ResultTupleSlot;
	TupleTableSlot	   *slot;
	ElimFilter		   *plan = (ElimFilter *) node->ss.ps.plan;

	for (;;)
	{
		switch (node->status)
		{
			case SS_INIT:
				node->status = SS_PROCESS;
				break;

			case SS_PROCESS:
				slot = ExecProcNode(outerPlanState(node));

				if (TupIsNull(slot))
				{
					node->status = SS_DONE;
					return NULL;
				}

				tuplewindow_rewind(window);
				if (node->flags & SL_FLAGS_RANKED)
					tuplewindow_setinsertrank(window, ExecSkylineRank(node, slot));
				for (;;)
				{
					int cmp;

					if (tuplewindow_ateof(window))
					{
						tuplewindow_puttupleslot(window, slot, 0, true);

						return slot;
					}

					tuplewindow_gettupleslot(window, inner_slot, false);

					cmp = ExecSkylineIsDominating(node, inner_slot, slot);

					/*
					 * The tuple in slot is dominated by a inner_slot in
					 * the window, so fetch the next.
					 */
					if (cmp == SKYLINE_CMP_FIRST_DOMINATES || (cmp == SKYLINE_CMP_ALL_EQ && plan->skyline_distinct))
						break;
					else if (cmp == SYKLINE_CMP_SECOND_DOMINATES)
					{
						tuplewindow_removecurrent(window);
					}
					else
					{
						tuplewindow_movenext(window);
					}
				}
				break;

			case SS_DONE:
				return NULL;

			default:
				/* Invalid State */
				AssertState(0);
 				return NULL;
		}
	}
}
