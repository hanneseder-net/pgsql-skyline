/*-------------------------------------------------------------------------
 *
 * nodeSkyline.c
 *	  Routines to handle skyline nodes (used for queries with SKYLINE BY clause).
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

#include "access/nbtree.h"
#include "executor/execdebug.h"
#include "executor/executor.h"
#include "executor/nodeSkyline.h"
#include "miscadmin.h"
#include "utils/datum.h"
#include "utils/tuplesort.h"
#include "utils/tuplestore.h"
#include "utils/tuplewindow.h"
#include "utils/lsyscache.h"
#include "utils/skyline.h"

/*
 * Inline-able copy of FunctionCall2() to save some cycles in sorting.
 */
/* TODO: this is from tublesort.c, export it there */
static inline Datum
myFunctionCall2(FmgrInfo *flinfo, Datum arg1, Datum arg2)
{
	FunctionCallInfoData fcinfo;
	Datum		result;

	InitFunctionCallInfoData(fcinfo, flinfo, 2, NULL, NULL);

	fcinfo.arg[0] = arg1;
	fcinfo.arg[1] = arg2;
	fcinfo.argnull[0] = false;
	fcinfo.argnull[1] = false;

	result = FunctionCallInvoke(&fcinfo);

	/* Check for null result, since caller is clearly not expecting one */
	if (fcinfo.isnull)
		elog(ERROR, "function %u returned NULL", fcinfo.flinfo->fn_oid);

	return result;
}

/*
 * Apply a compare (sort) function (by now converted to fmgr lookup form)
 * and return a 3-way comparison result.  This takes care of handling
 * reverse-sort and NULLs-ordering properly.  We assume that DESC and
 * NULLS_FIRST options are encoded in sk_flags the same way btree does it.
 */
/* TODO: this is from tublesort.c, export it there */
static inline int32
inlineApplyCompareFunction(FmgrInfo *compFunction, int sk_flags,
						   Datum datum1, bool isNull1,
						   Datum datum2, bool isNull2)
{
	int32		compare;

	if (isNull1)
	{
		if (isNull2)
			compare = 0;		/* NULL "=" NULL */
		else if (sk_flags & SK_BT_NULLS_FIRST)
			compare = -1;		/* NULL "<" NOT_NULL */
		else
			compare = 1;		/* NULL ">" NOT_NULL */
	}
	else if (isNull2)
	{
		if (sk_flags & SK_BT_NULLS_FIRST)
			compare = 1;		/* NOT_NULL ">" NULL */
		else
			compare = -1;		/* NOT_NULL "<" NULL */
	}
	else
	{
		compare = DatumGetInt32(myFunctionCall2(compFunction,
												datum1, datum2));

		if (sk_flags & SK_BT_DESC)
			compare = -compare;
	}

	return compare;
}

/*
 * ExecSkylineRank
 *
 *  FIXME
 *  HACK: this needs to depand on the stats of the columns in question
 */
double
ExecSkylineRank(SkylineState *node, TupleTableSlot *slot)
{
	Skyline    *sl = (Skyline *) node->ss.ps.plan;
	double		res = 0.0;
	int			i;

	for (i = 0; i < sl->numCols; ++i)
	{
		Datum		datum;
		double		value;
		bool		isnull;
		int			attnum = sl->skylineColIdx[i];

		datum = slot_getattr(slot, attnum, &isnull);

		/* HACK */
		if (isnull)
			return DBL_MAX;

		value = DatumGetFloat8(datum);

		if (value <= 1e-6)
			value = 1e-6;
		else if (value >= 1-1e-6)
			value = 1-1e-6;

		res += log(value);
	}

	return res;
}

/*
 * ExecSkylineIsDominating
 *
 *	FIXME
 */
int
ExecSkylineIsDominating(SkylineState *node, TupleTableSlot *inner_slot, TupleTableSlot *slot)
{
	Skyline    *sl = (Skyline *) node->ss.ps.plan;
	int			i;
	bool		cmp_all_eq = true;
	bool		cmp_lt = false;
	bool		cmp_gt = false;

	/* collect statistics */
	node->cmps_tuples++;

	for (i = 0; i < sl->numCols; ++i)
	{
		Datum		datum1;
		Datum		datum2;
		bool		isnull1;
		bool		isnull2;
		int			attnum = sl->skylineColIdx[i];
		int			cmp;

		/* collect statistics */
		node->cmps_fields++;

		datum1 = slot_getattr(inner_slot, attnum, &isnull1);
		datum2 = slot_getattr(slot, attnum, &isnull2);

		cmp = inlineApplyCompareFunction(&(node->compareOpFn[i]), node->compareFlags[i], datum1, isnull1, datum2, isnull2);

		cmp_all_eq &= (cmp == 0);

		switch (sl->skylineByDir[i])
		{
			case SKYLINEBY_DEFAULT:
			case SKYLINEBY_MIN:
			case SKYLINEBY_MAX:
			case SKYLINEBY_USING:
				if (cmp < 0)
				{
					cmp_lt = true;
					if (cmp_gt)
						return SKYLINE_CMP_INCOMPARABLE;
				}
				else if (cmp > 0)
				{
					cmp_gt = true;
					if (cmp_lt)
						return SKYLINE_CMP_INCOMPARABLE;
				}

				break;
			case SKYLINEBY_DIFF:
				/*
				 * FIXME: For SFS if we sort first on all the DIFF attrs
				 * then we could flush the tuple window in this case.
				 * Return SKYLINE_CMP_DIFF_GRP_DIFF to indicate this
				 *
				 * FIXME: we could use SKYLINE_CMP_DIFF_GRP_DIFF for all
				 * other methods in the same places as SKYLINE_CMP_DIFF_GRP_DIFF
				 */
				if (cmp != 0)
					return SKYLINE_CMP_INCOMPARABLE;
				break;
			default:
				elog(ERROR, "unrecognized skylineby_dir: %d", sl->skylineByDir[i]);
				break;
		}
	}

	if (cmp_all_eq)
		return SKYLINE_CMP_ALL_EQ;

	if (cmp_lt)
		return SKYLINE_CMP_FIRST_DOMINATES;

	if (cmp_gt)
		return SYKLINE_CMP_SECOND_DOMINATES;

	return SKYLINE_CMP_INCOMPARABLE;
}

/*
 * ExecSkylineCacheCompareFunctionInfo
 *
 *	FIXME
 */
void
ExecSkylineCacheCompareFunctionInfo(SkylineState *slstate, Skyline *node)
{
	int			i;

	slstate->compareOpFn = (FmgrInfo *) palloc(node->numCols * sizeof(FmgrInfo));
	slstate->compareFlags = (int *) palloc(node->numCols * sizeof(int));

	for (i = 0; i < node->numCols; ++i)
	{
		Oid			compareFunction;
		SelectSortFunction(node->skylinebyOperators[i], 
						   node->nullsFirst[i], 
						   &compareFunction,
						   &slstate->compareFlags[i]);
		fmgr_info(compareFunction, &(slstate->compareOpFn[i]));
	}
}

/*
 * ExecSkylineNeedExtraSlot
 *
 *	FIXME
 */
static bool
ExecSkylineNeedExtraSlot(Skyline *node)
{
	switch (node->skyline_method)
	{
		case SM_BLOCKNESTEDLOOP:
		case SM_SFS:
			return true;

		default:
			return false;
	}
}

/*
 * ExecSkylineInitTupleWindow
 *
 *	FIXME
 */
static void
ExecSkylineInitTupleWindow(SkylineState *node, Skyline *sl)
{
	int			window_size = work_mem;
	int			window_slots = -1;

	Assert(node != NULL);

	if (node->skyline_method == SM_BLOCKNESTEDLOOP ||
		node->skyline_method == SM_SFS)
	{
		if (node->window != NULL)
		{
			tuplewindow_end(node->window);
			node->window = NULL;
		}

		/*
		 * Can be overrided by an option, otherwise use entire
		 * work_mem.
		 */
		skyline_option_get_int(sl->skyline_by_options, "window", &window_size) ||
			skyline_option_get_int(sl->skyline_by_options, "windowsize", &window_size);

		skyline_option_get_int(sl->skyline_by_options, "slots", &window_slots) ||
			skyline_option_get_int(sl->skyline_by_options, "windowslots", &window_slots);

		if (window_slots == 0)
		{
			/*
			 * If window_slots == -1, then we constrain the tuple window in
			 * terms of memory.
			 */
			elog(ERROR, "tuple window must have at least one slot");
		}

		node->window = tuplewindow_begin(window_size, window_slots);

		node->windowsize = window_size;
		node->windowslots = window_slots;
	}
	else
	{
		node->windowsize = -1;
		node->windowslots = -1;
	}
}

/*
 * ExecInitSkyline
 *
 *	FIXME
 */
SkylineState *
ExecInitSkyline(Skyline *node, EState *estate, int eflags)
{
	SkylineState   *slstate;
	bool			need_extra_slot = ExecSkylineNeedExtraSlot(node);
	int				use_entropy;

	/* check for unsupported flags */
	Assert(!(eflags & (EXEC_FLAG_BACKWARD | EXEC_FLAG_MARK)));

	slstate = makeNode(SkylineState);
	slstate->ss.ps.plan = (Plan *) node;
	slstate->ss.ps.state = estate;

	slstate->status = SS_INIT;
	slstate->source = SS_OUTER;
	slstate->skyline_method = node->skyline_method;
	slstate->pass = 1;

	slstate->cmps_tuples = 0;
	slstate->cmps_fields = 0;
	slstate->pass_info = makeStringInfo();
	slstate->flags = 0;

	if (skyline_option_get_int( node->skyline_by_options, "entropy", &use_entropy))
	{
		slstate->flags |= SL_FLAGS_ENTROPY;
	}

	ExecSkylineInitTupleWindow(slstate, node);

	/*
	 * Miscellaneous initialization
	 *
	 * Skyline nodes don't initialize their ExprContexts because they never
	 * call ExecQual or ExecProject.
	 *
	 * FIXME: We could project the tuples in the tuple window for SFS.
	 *
	 * FIXME: We could project the tuples after the skyline if the
	 * skyline attribues are not part of the result.
	 */

	/*
	 * create expression context
	 */
	/* ExecAssignExprContext(estate, &slstate->ss.ps); */ /* see above */

	/*
	 * If ExecSkylineNeedExtraSlot(node) returns true, allocate one slot 
	 * more, see: ExecCountSlotsSkyline
	 */
#define SKYLINE_NSLOTS 2

	/*
	 * tuple table initialization
	 */
	ExecInitScanTupleSlot(estate, &slstate->ss);
	ExecInitResultTupleSlot(estate, &slstate->ss.ps);

	if (need_extra_slot)
	{
		/* for extra slot */
		slstate->extraSlot = ExecInitExtraTupleSlot(estate);
	}

	/*
	 * initialize child expressions
	 */
	slstate->ss.ps.targetlist = (List *)
		ExecInitExpr((Expr *) node->plan.targetlist,
					 (PlanState *) slstate);
	slstate->ss.ps.qual = (List *)
		ExecInitExpr((Expr *) node->plan.qual,
					 (PlanState *) slstate);

	/*
	 * initialize child nodes
	 */

	/*
	 * in case of the materialized nested loop, we need the outer plan to
	 * handle mark/rewind which is achived by an extra materialize node
	 */
	if (node->skyline_method == SM_MATERIALIZEDNESTEDLOOP)
		eflags |= EXEC_FLAG_REWIND | EXEC_FLAG_MARK;

	outerPlanState(slstate) = ExecInitNode(outerPlan(node), estate, eflags);

	/*
	 * Initialize tuple type.  
	 *
	 * FIXME: is the following right
	 * No need to initialize projection info because
	 * this node doesn't do projections.
	 */
	ExecAssignResultTypeFromTL(&slstate->ss.ps);
	ExecAssignScanTypeFromOuterPlan(&slstate->ss);

	if (need_extra_slot)
	{
		/* for extra slot */
		ExecSetSlotDescriptor(slstate->extraSlot, ExecGetResultType(outerPlanState(slstate)));
	}
	ExecAssignProjectionInfo(&slstate->ss.ps, NULL);

	ExecSkylineCacheCompareFunctionInfo(slstate, node);

	return slstate;
}

/*
 * ExecCountSlotsSkyline
 *
 *	FIXME
 */
int
ExecCountSlotsSkyline(Skyline *node)
{
	return ExecCountSlotsNode(outerPlan(node)) + SKYLINE_NSLOTS + (ExecSkylineNeedExtraSlot(node) ? 1 : 0);
}

/*
 * ExecSkyline_1DimDistinct
 *
 *	FIXME
 */
static TupleTableSlot *
ExecSkyline_1DimDistinct(SkylineState *node, Skyline *sl)
{
	switch (node->status)
	{
		case SS_INIT:
			{
				int			compareFlags = node->compareFlags[0];
				FmgrInfo	compareOpFn = node->compareOpFn[0];
				Datum		datum1;
				Datum		datum2;
				bool		isnull1;
				bool		isnull2;
				int			attnum = sl->skylineColIdx[0];
				TupleTableSlot *resultSlot = node->ss.ps.ps_ResultTupleSlot;

				for (;;)
				{
					/* CHECK_FOR_INTERRUPTS(); is done in ExecProcNode */
					TupleTableSlot *slot = ExecProcNode(outerPlanState(node));

					if (TupIsNull(slot))
						break;

					if (TupIsNull(resultSlot))
					{
						ExecCopySlot(resultSlot, slot);
						datum1 = slot_getattr(resultSlot, attnum, &isnull1);
					}
					else
					{
						datum2 = slot_getattr(slot, attnum, &isnull2);

						if (inlineApplyCompareFunction(&compareOpFn, compareFlags, datum1, isnull1, datum2, isnull2) > 0)
						{
							/*
							 * Using the result slot avoids copying of varlen
							 * attrs.
							 */
							ExecCopySlot(resultSlot, slot);
							datum1 = slot_getattr(resultSlot, attnum, &isnull1);
						}
					}
				}

				node->status = SS_DONE;
				return resultSlot;
			}

		case SS_DONE:
			return NULL;

		default:
			/* Invalid State */
			AssertState(0);
			return NULL;
	}
}

/*
 * ExecSkyline_1Dim
 *
 *	FIXME
 */
static TupleTableSlot *
ExecSkyline_1Dim(SkylineState *node, Skyline *sl)
{
	for (;;)
	{
		switch (node->status)
		{
			case SS_INIT:
				{
					int			compareFlags = node->compareFlags[0];
					FmgrInfo	compareOpFn = node->compareOpFn[0];
					Datum		datum1;
					Datum		datum2;
					bool		isnull1;
					bool		isnull2;
					bool		first = true;
					int			attnum = sl->skylineColIdx[0];
					TupleTableSlot *resultSlot = node->ss.ps.ps_ResultTupleSlot;
					int16		typlen;
					bool		typbyval;

					get_typlenbyval(resultSlot->tts_tupleDescriptor->attrs[sl->skylineColIdx[0] - 1]->atttypid, &typlen, &typbyval);

					node->tuplestorestate = tuplestore_begin_heap(false, false, work_mem);
					tuplestore_set_eflags(node->tuplestorestate, EXEC_FLAG_MARK);
					for (;;)
					{
						/* CHECK_FOR_INTERRUPTS(); is done in ExecProcNode */
						TupleTableSlot *slot = ExecProcNode(outerPlanState(node));

						if (TupIsNull(slot))
							break;

						if (first)
						{
							datum1 = datumCopy(slot_getattr(slot, attnum, &isnull1), typbyval, typlen);
							tuplestore_puttupleslot(node->tuplestorestate, slot);

							first = false;
						}
						else
						{
							int			cmp;

							datum2 = slot_getattr(slot, attnum, &isnull2);

							cmp = inlineApplyCompareFunction(&compareOpFn, compareFlags, datum1, isnull1, datum2, isnull2);

							if (cmp == 0)
							{
								tuplestore_puttupleslot(node->tuplestorestate, slot);
							}
							else if (cmp > 0)
							{
								if (DatumGetPointer(datum1) != NULL)
									datumFree(datum1, typbyval, typlen);
								datum1 = datumCopy(datum2, typbyval, typlen);
								isnull1 = isnull2;

								tuplestore_catchup(node->tuplestorestate);
								tuplestore_puttupleslot(node->tuplestorestate, slot);
							}
						}
					}

					if (DatumGetPointer(datum1) != NULL)
						datumFree(datum1, typbyval, typlen);

					node->status = SS_PIPEOUT;
				}

				/* fall through */

			case SS_PIPEOUT:
				Assert(node->tuplestorestate != NULL);

				if (tuplestore_gettupleslot(node->tuplestorestate, true, node->ss.ps.ps_ResultTupleSlot))
					return node->ss.ps.ps_ResultTupleSlot;
				else
				{
					tuplestore_end(node->tuplestorestate);
					node->tuplestorestate = NULL;

					node->status = SS_DONE;
					return NULL;
				}

			case SS_DONE:
				return NULL;

			default:
				/* Invalid State */
				AssertState(0);
				return NULL;
		}
	}
}

/*
 * ExecSkyline_2DimPreSort
 *
 *	FIXME
 */
static TupleTableSlot *
ExecSkyline_2DimPreSort(SkylineState *node, Skyline *sl)
{
	TupleTableSlot *resultSlot = node->ss.ps.ps_ResultTupleSlot;
	TupleTableSlot *slot;

	switch (node->status)
	{
		case SS_INIT:
			slot = ExecProcNode(outerPlanState(node));
			if (!TupIsNull(slot))
			{
				ExecCopySlot(resultSlot, slot);
				node->status = SS_PROCESS;
			}
			else
			{
				node->status = SS_DONE;
			}
			return slot;

		case SS_PROCESS:
			AssertState(!TupIsNull(resultSlot));
			for (;;)
			{
				int			cmp;

				/* CHECK_FOR_INTERRUPTS(); is done in ExecProcNode */
				slot = ExecProcNode(outerPlanState(node));
				if (TupIsNull(slot))
				{
					node->status = SS_DONE;
					return NULL;
				}

				cmp = ExecSkylineIsDominating(node, slot, resultSlot);

				if (cmp == SKYLINE_CMP_INCOMPARABLE || (cmp == SKYLINE_CMP_ALL_EQ && !sl->skyline_distinct))
				{
					ExecCopySlot(resultSlot, slot);
					return resultSlot;
				}
			}

		case SS_DONE:
			return NULL;

		default:
			/* Invalid State */
			AssertState(0);
 			return NULL;
	}
}

/*
 * ExecSkyline_MaterializedNestedLoop
 *
 *	FIXME
 */
static TupleTableSlot *
ExecSkyline_MaterializedNestedLoop(SkylineState *node, Skyline *sl)
{
	/* nested loop using materialize as outer plan */
	TupleTableSlot *resultSlot = node->ss.ps.ps_ResultTupleSlot;

	switch (node->status)
	{
		case SS_INIT:
			ExecMarkPos(outerPlanState(node));
			node->sl_pos = 0;
			node->status = SS_PROCESS;
			
			/* fall trough */

		case SS_PROCESS:
			for (;;)
			{
				TupleTableSlot *slot;
				int64		sl_innerpos = 0;

				ExecRestrPos(outerPlanState(node));
				slot = ExecProcNode(outerPlanState(node));
				node->sl_pos++;
				ExecMarkPos(outerPlanState(node));

				if (TupIsNull(slot))
				{
					node->status = SS_DONE;
					return NULL;
				}

				ExecCopySlot(resultSlot, slot);

				ExecReScan(outerPlanState(node), NULL);

				for (;;)
				{
					int			cmp;

					TupleTableSlot *inner_slot = ExecProcNode(outerPlanState(node));

					sl_innerpos++;

					if (TupIsNull(inner_slot))
					{
						/* the tuple in the resultSlot is not dominated, return it */
						return resultSlot;
					}

					/* is inner_slot dominating resultSlot? */
					cmp = ExecSkylineIsDominating(node, inner_slot, resultSlot);

					if (sl->skyline_distinct)
					{
						if (cmp == SKYLINE_CMP_ALL_EQ)
						{
							/*
							 * the inner tuple is before the result tuple, so don't
							 * output the result tuple
							 */
							if (sl_innerpos < node->sl_pos)
								break;
						}
					}

					if (cmp == SKYLINE_CMP_FIRST_DOMINATES)
						break;
				}
			}

			break;	/* never reached */

		case SS_DONE:
			return NULL;

		default:
			/* Invalid State */
			AssertState(0);
			return NULL;
	}
}

/*
 * ExecSkyline_BlockNestedLoop
 *
 *	FIXME
 */
static TupleTableSlot *
ExecSkyline_BlockNestedLoop(SkylineState *node, Skyline *sl)
{
	for (;;)
	{
		switch (node->status)
		{
			case SS_INIT:
				{
					node->source = SS_OUTER;
					node->tempIn = NULL;

					/*
					 * tempOut should go directly to a temporary file,
					 * therefore we set work_mem = 0.
					 */
					node->tempOut = tuplestore_begin_heap(false, false, 0);

					node->timestampIn = 0;
					node->timestampOut = 0;

					node->status = SS_PROCESS;
				}
				break;

			case SS_PROCESS:
				{
					TupleWindowState *window = node->window;

					TupleTableSlot *inner_slot = node->ss.ps.ps_ResultTupleSlot;
					TupleTableSlot *slot;

					Assert(node->source == SS_OUTER || node->source == SS_TEMP);
					if (node->source == SS_OUTER)
					{
						slot = ExecProcNode(outerPlanState(node));
					}
					else
					{
						/*
						 * We need to call CHECK_FOR_INTERRUPTS() here
						 * since if we are processing only from temp files,
						 * ExecProcNode is not called.
						 */
						CHECK_FOR_INTERRUPTS();

						slot = node->extraSlot;
						tuplestore_gettupleslot(node->tempIn, true, slot);
					}

					if (TupIsNull(slot))
					{
						if (node->source == SS_OUTER)
							appendStringInfo(node->pass_info, "%lld", node->timestampIn);

						/*
						 * If we have read all tuples for the outer node
						 * switch to temp (FIXME: comment is wrong)
						 */
						if (node->source == SS_TEMP)
						{
							tuplestore_end(node->tempIn);
							node->tempIn = NULL;
						}

						if (node->timestampOut == 0)
						{
							/*
							 * We haven't written any tuples to the temp, so
							 * we are done.
							 */
							tuplestore_end(node->tempOut);
							node->tempOut = NULL;

							tuplewindow_rewind(window);
							node->status = SS_FINALPIPEOUT;
						}
						else
						{
							node->pass++;
							appendStringInfo(node->pass_info, ", %lld", node->timestampOut);
							elog(DEBUG1, "start pass %lld with %lld tuples in temp", node->pass, node->timestampOut);

							node->source = SS_TEMP;
							node->tempIn = node->tempOut;

							/*
							 * tempOut should go directly to a temporary file,
							 * therefore we set work_mem = 0.
							 */
							node->tempOut = tuplestore_begin_heap(false, false, 0);

							node->timestampIn = 0;
							node->timestampOut = 0;

							tuplewindow_rewind(window);
							node->status = SS_PIPEOUT;
						}
						break;
					}

					node->timestampIn++;

					tuplewindow_rewind(window);
					if (node->flags & SL_FLAGS_ENTROPY)
						tuplewindow_setinsertrank(window, ExecSkylineRank(node, slot));
					for (;;)
					{
						int			cmp;

						if (tuplewindow_ateof(window))
						{
							/*
							 * The tuple in slot is not dominated, by one in
							 * the window put it in the window or write 
							 * it to temp.
							 */
							if (tuplewindow_has_freespace(window))
							{
								if (node->flags & SL_FLAGS_ENTROPY)
									tuplewindow_puttupleslotatinsertrank(window, slot, node->timestampOut);
								else
									tuplewindow_puttupleslot(window, slot, node->timestampOut);
							}
							else
							{
								tuplestore_puttupleslot(node->tempOut, slot);
								node->timestampOut++;
							}
							break;
						}

						tuplewindow_gettupleslot(window, inner_slot, false);

						cmp = ExecSkylineIsDominating(node, inner_slot, slot);

						/*
						 * The tuple in slot is dominated by a inner_slot in
						 * the window, so fetch the next.
						 */
						if (cmp == SKYLINE_CMP_FIRST_DOMINATES || (cmp == SKYLINE_CMP_ALL_EQ && sl->skyline_distinct))
							break;

						if (cmp == SYKLINE_CMP_SECOND_DOMINATES)
						{
							/*
							 * In case were we remove a tuple from the window,
						     * the window cursor (current) is move the the
							 * next by tuplewindow_removecurrent.
							 */
							tuplewindow_removecurrent(window);
						}
						else
						{
							tuplewindow_movenext(window);
						}
					}

					tuplewindow_rewind(window);
					node->status = SS_PIPEOUT;
				}

				break;

			case SS_PIPEOUT:

				/*
				 * Before switching to this state call
				 * tuplewindow_rewind(node->window);
				 */
				for (;;)
				{
					if (tuplewindow_ateof(node->window))
					{
						node->status = SS_PROCESS;
						break;
					}
					else
					{
						if (tuplewindow_timestampcurrent(node->window) == node->timestampIn)
						{
							TupleTableSlot *resultSlot = node->ss.ps.ps_ResultTupleSlot;

							tuplewindow_gettupleslot(node->window, resultSlot, true);
							return resultSlot;
						}
						else
						{
							tuplewindow_movenext(node->window);
						}

					}
				}
				break;

			case SS_FINALPIPEOUT:
				if (tuplewindow_ateof(node->window))
				{
					tuplewindow_end(node->window);
					node->status = SS_DONE;
					return NULL;
				}
				else
				{
					TupleTableSlot *resultSlot = node->ss.ps.ps_ResultTupleSlot;

					tuplewindow_gettupleslot(node->window, resultSlot, true);
					return resultSlot;
				}

			case SS_DONE:
				return NULL;

			default:
				/* Invalid State */
				AssertState(0);
				return NULL;
		}
	}
}

/*
 * ExecSkyline_SortFilterSkyline
 *
 *	FIXME
 */
static TupleTableSlot *
ExecSkyline_SortFilterSkyline(SkylineState *node, Skyline *sl)
{
	for (;;)
	{
		switch (node->status)
		{
			case SS_INIT:
				{
					node->source = SS_OUTER;
					node->tempIn = NULL;

					/*
					 * tempOut should go directly to a temporary file,
					 * therefore we set work_mem = 0.
					 */
					node->tempOut = tuplestore_begin_heap(false, false, 0);

					node->timestampOut = 0;

					node->status = SS_PROCESS;
				}
				break;

			case SS_PROCESS:
				{
					TupleWindowState *window = node->window;

					TupleTableSlot *inner_slot = node->ss.ps.ps_ResultTupleSlot;
					TupleTableSlot *slot;

					Assert(node->source == SS_OUTER || node->source == SS_TEMP);
					if (node->source == SS_OUTER)
					{
						slot = ExecProcNode(outerPlanState(node));
					}
					else
					{
						/*
						 * We need to call CHECK_FOR_INTERRUPTS() here
						 * since if we are processing only from temp files,
						 * ExecProcNode is not called
						 */
						CHECK_FOR_INTERRUPTS();

						slot = node->extraSlot;
						tuplestore_gettupleslot(node->tempIn, true, slot);
					}

					if (TupIsNull(slot))
					{
						if (node->source == SS_OUTER)
							appendStringInfo(node->pass_info, "%lld", node->timestampIn);

						/*
						 * If we have read all tuples for the outer node
						 * switch to temp (FIXME: comment is wrong)
						 */
						if (node->source == SS_TEMP)
						{
							tuplestore_end(node->tempIn);
							node->tempIn = NULL;
						}

						if (node->timestampOut == 0)
						{
							/*
							 * We haven't written any tuples to the temp, so
							 * we are done.
							 */
							tuplestore_end(node->tempOut);
							node->tempOut = NULL;

							node->status = SS_DONE;
							return NULL;
						}
						else
						{
							node->pass++;
							appendStringInfo(node->pass_info, ", %lld", node->timestampOut);
							elog(DEBUG1, "start pass %lld with %lld tuples in temp", node->pass, node->timestampOut);

							node->source = SS_TEMP;
							node->tempIn = node->tempOut;

							/*
							 * By using work_mem = 0 we force the tuples in
							 * the tuplestore to go directly to the temp
							 * file
							 */
							node->tempOut = tuplestore_begin_heap(false, false, 0 /* work_mem */);

							node->timestampOut = 0;

							/*
							 * We just clean the window here, the tuples have
							 * allready been piped out.
							 */
							tuplewindow_clean(window);
						}
						break;
					}

					node->timestampIn++;

					tuplewindow_rewind(window);
					if (node->flags & SL_FLAGS_ENTROPY)
						tuplewindow_setinsertrank(window, ExecSkylineRank(node, slot));
					for (;;)
					{
						int			cmp;

						if (tuplewindow_ateof(window))
						{
							/*
							 * The tuple in the slot is not dominated by one
							 * in the window, so put into the window and pipe 
							 * it out or write it to temp, if the window does
							 * not have enough free space.
							 */ 
							if (tuplewindow_has_freespace(window))
							{
								if (node->flags & SL_FLAGS_ENTROPY)
									tuplewindow_puttupleslotatinsertrank(window, slot, 0);
								else
									tuplewindow_puttupleslot(window, slot, 0);

								/*
								 * We can pipe out the tuple here.
								 */
								return slot;
							}
							else
							{
								tuplestore_puttupleslot(node->tempOut, slot);
								node->timestampOut++;
							}
							break;
						}

						tuplewindow_gettupleslot(window, inner_slot, false);

						cmp = ExecSkylineIsDominating(node, inner_slot, slot);

						/*
						 * The tuple in slot is dominated by a inner_slot in
						 * the window, so fetch the next.
						 */
						if (cmp == SKYLINE_CMP_FIRST_DOMINATES || (cmp == SKYLINE_CMP_ALL_EQ && sl->skyline_distinct))
							break;

						Assert(cmp != SYKLINE_CMP_SECOND_DOMINATES);
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
		}						/* switch */
	}							/* for */
}

/*
 * ExecSkyline
 *
 *	FIXME
 */
TupleTableSlot *
ExecSkyline(SkylineState *node)
{
	Skyline    *sl = (Skyline *) node->ss.ps.plan;

	switch (node->skyline_method)
	{
		case SM_1DIM:
			return ExecSkyline_1Dim(node, sl);
		case SM_1DIM_DISTINCT:
			return ExecSkyline_1DimDistinct(node, sl);
		case SM_2DIM_PRESORT:
			return ExecSkyline_2DimPreSort(node, sl);
		case SM_MATERIALIZEDNESTEDLOOP:
			return ExecSkyline_MaterializedNestedLoop(node, sl);
		case SM_BLOCKNESTEDLOOP:
			return ExecSkyline_BlockNestedLoop(node, sl);
		case SM_SFS:
			return ExecSkyline_SortFilterSkyline(node, sl);
		default:
			/* Invalid Skyline Method */
			AssertState(0);
			return NULL;
	}
}

/*
 * ExecEndSkyline
 *
 *	FIXME
 */
void
ExecEndSkyline(SkylineState *node)
{
	/*
	 * clean out the tuple table
	 */
	ExecClearTuple(node->ss.ss_ScanTupleSlot);
	ExecClearTuple(node->ss.ps.ps_ResultTupleSlot);
	if (ExecSkylineNeedExtraSlot((Skyline *)node->ss.ps.plan))
	{
		ExecClearTuple(node->extraSlot);
	}

	/*
	 * shut down the subplan
	 */
	ExecEndNode(outerPlanState(node));
}

/*
 * ExecReScanSkyline
 *
 *	FIXME
 */
void
ExecReScanSkyline(SkylineState *node, ExprContext *exprCtxt)
{
	/* FIXME: code coverage = 0 !!! */

	node->status = SS_INIT;

	/* must clear first tuple */
	ExecClearTuple(node->ss.ss_ScanTupleSlot);

	if (((PlanState *) node)->lefttree &&
		((PlanState *) node)->lefttree->chgParam == NULL)
		ExecReScan(((PlanState *) node)->lefttree, exprCtxt);

	node->status = SS_OUTER;
	node->pass = 1;

	node->cmps_tuples = 0;
	node->cmps_fields = 0;
	resetStringInfo(node->pass_info);

	/* reinit tuple window */
	ExecSkylineInitTupleWindow(node, (Skyline *) node->ss.ps.plan);


	/* close temp files */
	if (node->tempIn)
	{
		tuplestore_end(node->tempIn);
		node->tempIn = NULL;
	}

	if (node->tempOut)
	{
		tuplestore_end(node->tempOut);
		node->tempOut = NULL;
	}

	/* FIXME: there is maybe more that need to be done here */
}
