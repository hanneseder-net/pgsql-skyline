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
ExecElimFilterInitTupleWindow(SkylineState *state, Skyline *node)
{
	int			window_size = BLCKSZ / 1024;	/* allocate a page */
	int			window_slots = -1;
	TupleWindowPolicy	window_policy = TUP_WIN_POLICY_APPEND;

	Assert(state != NULL);

	if (state->window != NULL)
	{
		tuplewindow_end(state->window);
		state->window = NULL;
	}

	/*
	 * Can be overrided by an option, otherwise use entire
	 * work_mem.
	 */
	skyline_option_get_int(node->skyline_of_options, "efwindow", &window_size) ||
		skyline_option_get_int(node->skyline_of_options, "efwindowsize", &window_size);

	skyline_option_get_int(node->skyline_of_options, "efslots", &window_slots) ||
		skyline_option_get_int(node->skyline_of_options, "efwindowslots", &window_slots);

	if (window_slots == 0)
	{
		/*
		 * If window_slots == -1, then we constrain the tuple window in
		 * terms of memory.
		 */
		elog(ERROR, "tuple window must have at least one slot");
	}

	state->window = tuplewindow_begin(window_size, window_slots, state->window_policy);

	state->windowsize = window_size;
	state->windowslots = window_slots;
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
	state->skyline_method = node->skyline_method;
	state->pass = 1;

	state->cmps_tuples = 0;
	state->cmps_fields = 0;
	state->pass_info = makeStringInfo();
	state->flags = SL_FLAGS_NONE;
	state->window_policy = TUP_WIN_POLICY_APPEND;

	skyline_option_get_window_policy(node->skyline_of_options, "efwindowpolicy", &state->window_policy);

	/*
	 * If we do not have stats for at least one column, fall back from
	 * window policy "ranked" back to "append".
	 */
	if (state->window_policy == TUP_WIN_POLICY_RANKED && !(node->flags & SKYLINE_FLAGS_HAVE_STATS))
	{
		state->window_policy = TUP_WIN_POLICY_APPEND;
		elog(INFO, "no stats for skyline expressions available, falling back to window policy \"append\"");
	}

	if (state->window_policy == TUP_WIN_POLICY_RANKED)
		node->flags |= SL_FLAGS_RANKED;

	ExecElimFilterInitTupleWindow(state, node);

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
	ExecSkylineCacheCoerceFunctionInfo(state, node);

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
ExecEndElimFilter(ElimFilterState *state)
{
	/*
	 * clean out the tuple table
	 */
	ExecClearTuple(state->ss.ss_ScanTupleSlot);
	ExecClearTuple(state->ss.ps.ps_ResultTupleSlot);

	/*
	 * shut down the subplan
	 */
	ExecEndNode(outerPlanState(state));
}

/*
 * ExecReScanElimFilter
 *
 *	FIXME
 */
void
ExecReScanElimFilter(ElimFilterState *state, ExprContext *exprCtxt)
{
	/* FIXME: code coverage = 0 !!! */

	state->status = SS_INIT;

	/* must clear first tuple */
	ExecClearTuple(state->ss.ss_ScanTupleSlot);

	if (((PlanState *) state)->lefttree &&
		((PlanState *) state)->lefttree->chgParam == NULL)
		ExecReScan(((PlanState *) state)->lefttree, exprCtxt);

	/* reinit tuple window */
	ExecElimFilterInitTupleWindow(state, (Skyline *) state->ss.ps.plan);

	/* FIXME: there is maybe more that need to be done here */
}

/*
 * ExecElimFilter
 *
 *	FIXME
 */
TupleTableSlot *
ExecElimFilter(ElimFilterState *state)
{
	TupleWindowState   *window = state->window;
	TupleTableSlot	   *inner_slot = state->ss.ps.ps_ResultTupleSlot;
	TupleTableSlot	   *slot;
	ElimFilter		   *plan = (ElimFilter *) state->ss.ps.plan;

	for (;;)
	{
		switch (state->status)
		{
			case SS_INIT:
				state->status = SS_PROCESS;
				break;

			case SS_PROCESS:
				slot = ExecProcNode(outerPlanState(state));

				if (TupIsNull(slot))
				{
					state->status = SS_DONE;
					return NULL;
				}

				tuplewindow_rewind(window);
				if (state->flags & SL_FLAGS_RANKED)
					tuplewindow_setinsertrank(window, ExecSkylineRank(state, slot));
				for (;;)
				{
					int cmp;

					if (tuplewindow_ateof(window))
					{
						tuplewindow_puttupleslot(window, slot, 0, true);

						return slot;
					}

					tuplewindow_gettupleslot(window, inner_slot, false);

					cmp = ExecSkylineIsDominating(state, inner_slot, slot);

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
