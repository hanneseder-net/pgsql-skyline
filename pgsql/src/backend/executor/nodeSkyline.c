/*-------------------------------------------------------------------------
 *
 * nodeSkyline.c
 *	  Routines to handle skyline nodes (used for queries with SKYLINE OF clause).
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
 * ExecSkylineIsDominating
 *
 *	FIXME
 */
int
ExecSkylineIsDominating(SkylineState *state, TupleTableSlot *inner_slot, TupleTableSlot *slot)
{
	Skyline    *node = (Skyline *) state->ss.ps.plan;
	int			i;
	bool		cmp_all_eq = true;
	bool		cmp_lt = false;
	bool		cmp_gt = false;

	/* collect statistics */
	state->cmps_tuples++;

	for (i = 0; i < node->numCols; ++i)
	{
		Datum		datum1;
		Datum		datum2;
		bool		isnull1;
		bool		isnull2;
		int			attnum = node->skylineColIdx[i];
		int			cmp;

		/* collect statistics */
		state->cmps_fields++;

		datum1 = slot_getattr(inner_slot, attnum, &isnull1);
		datum2 = slot_getattr(slot, attnum, &isnull2);

		cmp = inlineApplyCompareFunction(&(state->compareOpFn[i]), state->compareFlags[i], datum1, isnull1, datum2, isnull2);

		cmp_all_eq &= (cmp == 0);

		switch (node->skylineOfDir[i])
		{
			case SKYLINEOF_DEFAULT:
			case SKYLINEOF_MIN:
			case SKYLINEOF_MAX:
			case SKYLINEOF_USING:
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
			case SKYLINEOF_DIFF:
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
				elog(ERROR, "unrecognized skylineof_dir: %d", node->skylineOfDir[i]);
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
 * ExecSkylineRank
 *
 *  FIXME
 */

double
ExecSkylineRank(SkylineState *state, TupleTableSlot *slot)
{
	Skyline    *node = (Skyline *) state->ss.ps.plan;
	double		res = 0.0;
	int			i;

	for (i = 0; i < node->numCols; ++i)
	{
		Datum		datum;
		double		value;
		bool		isnull;
		int			attnum;
		int			sk_flags;

		/* DIFF does not count for ranking */
		if (node->skylineOfDir[i] == SKYLINEOF_DIFF)
			continue;

		/*
		 * If we do not have stats, than we can't scale the column, so
		 * skip it.
		 */
		if ((node->colFlags[i] & SKYLINE_FLAGS_HAVE_STATS) == 0)
			continue;

		attnum =  node->skylineColIdx[i];
		sk_flags = state->compareFlags[i];

		datum = slot_getattr(slot, attnum, &isnull);

		if (isnull)
		{
			if (sk_flags & SK_BT_NULLS_FIRST)
				value = SKYLINE_RANK_BOUND_MIN;
			else
				value = SKYLINE_RANK_BOUND_MAX;
		}
		else
		{
			/*
			 * Coerce the datum into FLOAT8OID first if needed.
			 */
			if (node->colFlags[i] & SKYLINE_FLAGS_COERCE_FUNC)
			{
				FunctionCallInfoData fcinfo;
				InitFunctionCallInfoData(fcinfo, &state->coerceFn[i], 1, NULL, NULL);
				fcinfo.arg[0] = datum;
				fcinfo.argnull[0] = false;
				datum = FunctionCallInvoke(&fcinfo);
				/* Check for null result */
				if (fcinfo.isnull)
					elog(ERROR, "function %u returned NULL", state->coerceFn[i].fn_oid);
			}

			value = DatumGetFloat8(datum);

			/*
			 * Shift and scale the value into the interval
			 * [SKYLINE_RANK_BOUND_MIN, SKYLINE_RANK_BOUND_MAX].
			 */
			value -= node->colMin[i];
			value *= node->colScale[i];

			/* 
			 * If we do not have very good estimations for the range, make sure that
			 * 'value' is with the range [SKYLINE_RANK_BOUND_MIN, SKYLINE_RANK_BOUND_MAX].
			 */
			if (value <= SKYLINE_RANK_BOUND_MIN)
				value = SKYLINE_RANK_BOUND_MIN;
			else if (value >= SKYLINE_RANK_BOUND_MAX)
				value = SKYLINE_RANK_BOUND_MAX;
		}

		if (sk_flags & SK_BT_DESC)
			value = SKYLINE_RANK_BOUND_MAX - value;

		res -= log(value);
	}

	return res;
}

/*
 * ExecSkylineRandom
 *
 *  Returns a random number within [0.0, 1.0).
 */
double
ExecSkylineRandom()
{
	return (double) random() / ((double) MAX_RANDOM_VALUE + 1);
}

/*
 * ExecSkylineCacheCompareFunctionInfo
 *
 *	FIXME
 */
void
ExecSkylineCacheCompareFunctionInfo(SkylineState *state, Skyline *node)
{
	int			i;

	state->compareOpFn = (FmgrInfo *) palloc(node->numCols * sizeof(FmgrInfo));
	state->compareFlags = (int *) palloc(node->numCols * sizeof(int));

	for (i = 0; i < node->numCols; ++i)
	{
		Oid			compareFunction;
		SelectSortFunction(node->skylineOfOperators[i], 
						   node->nullsFirst[i], 
						   &compareFunction,
						   &state->compareFlags[i]);
		fmgr_info(compareFunction, &(state->compareOpFn[i]));
	}
}

/*
 * ExecSkylineCacheCoerceFunctionInfo
 *
 *  FIXME
 */
void
ExecSkylineCacheCoerceFunctionInfo(SkylineState *state, Skyline *node)
{
	int			i;

	state->coerceFn = (FmgrInfo *) palloc(node->numCols * sizeof(FmgrInfo));

	for (i = 0; i < node->numCols; ++i)
	{
		if (node->colCoerceFunc[i] != InvalidOid)
			fmgr_info(node->colCoerceFunc[i], &(state->coerceFn[i]));
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
ExecSkylineInitTupleWindow(SkylineState *state, Skyline *node)
{
	int			window_size = work_mem;
	int			window_slots = -1;

	Assert(state != NULL);

	if (state->skyline_method == SM_BLOCKNESTEDLOOP
		|| state->skyline_method == SM_SFS)
	{
		if (state->window != NULL)
		{
			tuplewindow_end(state->window);
			state->window = NULL;
		}

		/*
		 * Can be overrided by an option, otherwise use entire
		 * work_mem.
		 */
		skyline_option_get_int(node->skyline_of_options, "window", &window_size) ||
			skyline_option_get_int(node->skyline_of_options, "windowsize", &window_size);

		skyline_option_get_int(node->skyline_of_options, "slots", &window_slots) ||
			skyline_option_get_int(node->skyline_of_options, "windowslots", &window_slots);

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
	else
	{
		state->windowsize = -1;
		state->windowslots = -1;
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
	SkylineState   *state;
	bool			need_extra_slot = ExecSkylineNeedExtraSlot(node);

	/* check for unsupported flags */
	Assert(!(eflags & (EXEC_FLAG_BACKWARD | EXEC_FLAG_MARK)));

	state = makeNode(SkylineState);
	state->ss.ps.plan = (Plan *) node;
	state->ss.ps.state = estate;

	state->status = SS_INIT;
	state->source = SS_OUTER;
	state->skyline_method = node->skyline_method;
	state->pass = 1;

	state->cmps_tuples = 0;
	state->cmps_fields = 0;
	state->pass_info = makeStringInfo();
	state->flags = SL_FLAGS_NONE;
	state->window_policy = TUP_WIN_POLICY_APPEND;

	skyline_option_get_window_policy(node->skyline_of_options, "windowpolicy", &state->window_policy);

	/*
	 * If we do not have stats for at least one column, fall back from
	 * window policy "ranked" back to "append".
	 */
	if (state->window_policy == TUP_WIN_POLICY_ENTROPY && !(node->flags & SKYLINE_FLAGS_HAVE_STATS))
	{
		state->window_policy = TUP_WIN_POLICY_APPEND;
		elog(INFO, "no stats for skyline expressions available, falling back to window policy \"append\"");
	}

	if (state->window_policy == TUP_WIN_POLICY_ENTROPY)
		state->flags |= SL_FLAGS_RANKED | SL_FLAGS_ENTROPY;
	else if (state->window_policy == TUP_WIN_POLICY_RANDOM)
		state->flags |= SL_FLAGS_RANKED | SL_FLAGS_RANDOM;

	ExecSkylineInitTupleWindow(state, node);

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
	ExecInitScanTupleSlot(estate, &state->ss);
	ExecInitResultTupleSlot(estate, &state->ss.ps);

	if (need_extra_slot)
	{
		/* for extra slot */
		state->extraSlot = ExecInitExtraTupleSlot(estate);
	}

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

	/*
	 * in case of the materialized nested loop, we need the outer plan to
	 * handle mark/rewind which is achived by an extra materialize node
	 */
	if (node->skyline_method == SM_MATERIALIZEDNESTEDLOOP)
		eflags |= EXEC_FLAG_REWIND | EXEC_FLAG_MARK;

	outerPlanState(state) = ExecInitNode(outerPlan(node), estate, eflags);

	/*
	 * Initialize tuple type.  
	 *
	 * FIXME: is the following right
	 * No need to initialize projection info because
	 * this node doesn't do projections.
	 */
	ExecAssignResultTypeFromTL(&state->ss.ps);
	ExecAssignScanTypeFromOuterPlan(&state->ss);

	if (need_extra_slot)
	{
		/* for extra slot */
		ExecSetSlotDescriptor(state->extraSlot, ExecGetResultType(outerPlanState(state)));
	}
	ExecAssignProjectionInfo(&state->ss.ps, NULL);

	ExecSkylineCacheCompareFunctionInfo(state, node);
	ExecSkylineCacheCoerceFunctionInfo(state, node);

	return state;
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
ExecSkyline_1DimDistinct(SkylineState *state, Skyline *node)
{
	switch (state->status)
	{
		case SS_INIT:
			{
				int			compareFlags = state->compareFlags[0];
				FmgrInfo	compareOpFn = state->compareOpFn[0];
				Datum		datum1;
				Datum		datum2;
				bool		isnull1;
				bool		isnull2;
				int			attnum = node->skylineColIdx[0];
				TupleTableSlot *resultSlot = state->ss.ps.ps_ResultTupleSlot;

				for (;;)
				{
					/* CHECK_FOR_INTERRUPTS(); is done in ExecProcNode */
					TupleTableSlot *slot = ExecProcNode(outerPlanState(state));

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

				state->status = SS_DONE;
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
ExecSkyline_1Dim(SkylineState *state, Skyline *node)
{
	for (;;)
	{
		switch (state->status)
		{
			case SS_INIT:
				{
					int			compareFlags = state->compareFlags[0];
					FmgrInfo	compareOpFn = state->compareOpFn[0];
					Datum		datum1;
					Datum		datum2;
					bool		isnull1;
					bool		isnull2;
					bool		first = true;
					int			attnum = node->skylineColIdx[0];
					TupleTableSlot *resultSlot = state->ss.ps.ps_ResultTupleSlot;
					int16		typlen;
					bool		typbyval;

					get_typlenbyval(resultSlot->tts_tupleDescriptor->attrs[node->skylineColIdx[0] - 1]->atttypid, &typlen, &typbyval);

					state->tuplestorestate = tuplestore_begin_heap(false, false, work_mem);
					tuplestore_set_eflags(state->tuplestorestate, EXEC_FLAG_MARK);
					for (;;)
					{
						/* CHECK_FOR_INTERRUPTS(); is done in ExecProcNode */
						TupleTableSlot *slot = ExecProcNode(outerPlanState(state));

						if (TupIsNull(slot))
							break;

						if (first)
						{
							datum1 = datumCopy(slot_getattr(slot, attnum, &isnull1), typbyval, typlen);
							tuplestore_puttupleslot(state->tuplestorestate, slot);

							first = false;
						}
						else
						{
							int			cmp;

							datum2 = slot_getattr(slot, attnum, &isnull2);

							cmp = inlineApplyCompareFunction(&compareOpFn, compareFlags, datum1, isnull1, datum2, isnull2);

							if (cmp == 0)
							{
								tuplestore_puttupleslot(state->tuplestorestate, slot);
							}
							else if (cmp > 0)
							{
								if (DatumGetPointer(datum1) != NULL)
									datumFree(datum1, typbyval, typlen);
								datum1 = datumCopy(datum2, typbyval, typlen);
								isnull1 = isnull2;

								tuplestore_catchup(state->tuplestorestate);
								tuplestore_puttupleslot(state->tuplestorestate, slot);
							}
						}
					}

					if (DatumGetPointer(datum1) != NULL)
						datumFree(datum1, typbyval, typlen);

					state->status = SS_PIPEOUT;
				}

				/* fall through */

			case SS_PIPEOUT:
				Assert(state->tuplestorestate != NULL);

				if (tuplestore_gettupleslot(state->tuplestorestate, true, state->ss.ps.ps_ResultTupleSlot))
					return state->ss.ps.ps_ResultTupleSlot;
				else
				{
					tuplestore_end(state->tuplestorestate);
					state->tuplestorestate = NULL;

					state->status = SS_DONE;
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
ExecSkyline_2DimPreSort(SkylineState *state, Skyline *node)
{
	TupleTableSlot *resultSlot = state->ss.ps.ps_ResultTupleSlot;
	TupleTableSlot *slot;

	switch (state->status)
	{
		case SS_INIT:
			slot = ExecProcNode(outerPlanState(state));
			if (!TupIsNull(slot))
			{
				ExecCopySlot(resultSlot, slot);
				state->status = SS_PROCESS;
			}
			else
			{
				state->status = SS_DONE;
			}
			return slot;

		case SS_PROCESS:
			AssertState(!TupIsNull(resultSlot));
			for (;;)
			{
				int			cmp;

				/* CHECK_FOR_INTERRUPTS(); is done in ExecProcNode */
				slot = ExecProcNode(outerPlanState(state));
				if (TupIsNull(slot))
				{
					state->status = SS_DONE;
					return NULL;
				}

				cmp = ExecSkylineIsDominating(state, slot, resultSlot);

				if (cmp == SKYLINE_CMP_INCOMPARABLE || (cmp == SKYLINE_CMP_ALL_EQ && !node->skyline_distinct))
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
ExecSkyline_MaterializedNestedLoop(SkylineState *state, Skyline *node)
{
	/* nested loop using materialize as outer plan */
	TupleTableSlot *resultSlot = state->ss.ps.ps_ResultTupleSlot;

	switch (state->status)
	{
		case SS_INIT:
			ExecMarkPos(outerPlanState(state));
			state->sl_pos = 0;
			state->status = SS_PROCESS;
			
			/* fall trough */

		case SS_PROCESS:
			for (;;)
			{
				TupleTableSlot *slot;
				int64		sl_innerpos = 0;

				ExecRestrPos(outerPlanState(state));
				slot = ExecProcNode(outerPlanState(state));
				state->sl_pos++;
				ExecMarkPos(outerPlanState(state));

				if (TupIsNull(slot))
				{
					state->status = SS_DONE;
					return NULL;
				}

				ExecCopySlot(resultSlot, slot);

				ExecReScan(outerPlanState(state), NULL);

				for (;;)
				{
					int			cmp;

					TupleTableSlot *inner_slot = ExecProcNode(outerPlanState(state));

					sl_innerpos++;

					if (TupIsNull(inner_slot))
					{
						/* the tuple in the resultSlot is not dominated, return it */
						return resultSlot;
					}

					/* is inner_slot dominating resultSlot? */
					cmp = ExecSkylineIsDominating(state, inner_slot, resultSlot);

					if (node->skyline_distinct)
					{
						if (cmp == SKYLINE_CMP_ALL_EQ)
						{
							/*
							 * the inner tuple is before the result tuple, so don't
							 * output the result tuple
							 */
							if (sl_innerpos < state->sl_pos)
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
ExecSkyline_BlockNestedLoop(SkylineState *state, Skyline *node)
{
	for (;;)
	{
		switch (state->status)
		{
			case SS_INIT:
				{
					state->source = SS_OUTER;
					state->tempIn = NULL;

					/*
					 * tempOut should go directly to a temporary file,
					 * therefore we set work_mem = 0.
					 */
					state->tempOut = tuplestore_begin_heap(false, false, 0);

					state->timestampIn = 0;
					state->timestampOut = 0;

					state->status = SS_PROCESS;
				}
				break;

			case SS_PROCESS:
				{
					TupleWindowState *window = state->window;

					TupleTableSlot *inner_slot = state->ss.ps.ps_ResultTupleSlot;
					TupleTableSlot *slot;

					Assert(state->source == SS_OUTER || state->source == SS_TEMP);
					if (state->source == SS_OUTER)
					{
						slot = ExecProcNode(outerPlanState(state));
					}
					else
					{
						/*
						 * We need to call CHECK_FOR_INTERRUPTS() here
						 * since if we are processing only from temp files,
						 * ExecProcNode is not called.
						 */
						CHECK_FOR_INTERRUPTS();

						slot = state->extraSlot;
						tuplestore_gettupleslot(state->tempIn, true, slot);
					}

					if (TupIsNull(slot))
					{
						if (state->source == SS_OUTER)
							appendStringInfo(state->pass_info, "%lld", state->timestampIn);

						/*
						 * If we have read all tuples for the outer node
						 * switch to temp (FIXME: comment is wrong)
						 */
						if (state->source == SS_TEMP)
						{
							tuplestore_end(state->tempIn);
							state->tempIn = NULL;
						}

						if (state->timestampOut == 0)
						{
							/*
							 * We haven't written any tuples to the temp, so
							 * we are done.
							 */
							tuplestore_end(state->tempOut);
							state->tempOut = NULL;

							tuplewindow_rewind(window);
							state->status = SS_FINALPIPEOUT;
						}
						else
						{
							state->pass++;
							appendStringInfo(state->pass_info, ", %lld", state->timestampOut);
							elog(DEBUG1, "start pass %lld with %lld tuples in temp", state->pass, state->timestampOut);

							state->source = SS_TEMP;
							state->tempIn = state->tempOut;

							/*
							 * tempOut should go directly to a temporary file,
							 * therefore we set work_mem = 0.
							 */
							state->tempOut = tuplestore_begin_heap(false, false, 0);

							state->timestampIn = 0;
							state->timestampOut = 0;

							tuplewindow_rewind(window);
							state->status = SS_PIPEOUT;
						}
						break;
					}

					state->timestampIn++;

					tuplewindow_rewind(window);
					if (state->flags & SL_FLAGS_RANKED)
					{
						if (state->flags & SL_FLAGS_ENTROPY)
							tuplewindow_setinsertrank(window, ExecSkylineRank(state, slot));
						else if (state->flags & SL_FLAGS_RANDOM)
							tuplewindow_setinsertrank(window, ExecSkylineRandom());
						else
							Assert(0);
					}
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
								tuplewindow_puttupleslot(window, slot, state->timestampOut, false);
							}
							else
							{
								tuplestore_puttupleslot(state->tempOut, slot);
								state->timestampOut++;
							}
							break;
						}

						tuplewindow_gettupleslot(window, inner_slot, false);

						cmp = ExecSkylineIsDominating(state, inner_slot, slot);

						/*
						 * The tuple in slot is dominated by a inner_slot in
						 * the window, so fetch the next.
						 */
						if (cmp == SKYLINE_CMP_FIRST_DOMINATES || (cmp == SKYLINE_CMP_ALL_EQ && node->skyline_distinct))
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
					state->status = SS_PIPEOUT;
				}

				break;

			case SS_PIPEOUT:

				/*
				 * Before switching to this state call
				 * tuplewindow_rewind(node->window);
				 */
				for (;;)
				{
					if (tuplewindow_ateof(state->window))
					{
						state->status = SS_PROCESS;
						break;
					}
					else
					{
						if (tuplewindow_timestampcurrent(state->window) == state->timestampIn)
						{
							TupleTableSlot *resultSlot = state->ss.ps.ps_ResultTupleSlot;

							tuplewindow_gettupleslot(state->window, resultSlot, true);
							return resultSlot;
						}
						else
						{
							tuplewindow_movenext(state->window);
						}

					}
				}
				break;

			case SS_FINALPIPEOUT:
				if (tuplewindow_ateof(state->window))
				{
					tuplewindow_end(state->window);
					state->status = SS_DONE;
					return NULL;
				}
				else
				{
					TupleTableSlot *resultSlot = state->ss.ps.ps_ResultTupleSlot;

					tuplewindow_gettupleslot(state->window, resultSlot, true);
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
ExecSkyline_SortFilterSkyline(SkylineState *state, Skyline *node)
{
	for (;;)
	{
		switch (state->status)
		{
			case SS_INIT:
				{
					state->source = SS_OUTER;
					state->tempIn = NULL;

					/*
					 * tempOut should go directly to a temporary file,
					 * therefore we set work_mem = 0.
					 */
					state->tempOut = tuplestore_begin_heap(false, false, 0);

					state->timestampOut = 0;

					state->status = SS_PROCESS;
				}
				break;

			case SS_PROCESS:
				{
					TupleWindowState *window = state->window;

					TupleTableSlot *inner_slot = state->ss.ps.ps_ResultTupleSlot;
					TupleTableSlot *slot;

					Assert(state->source == SS_OUTER || state->source == SS_TEMP);
					if (state->source == SS_OUTER)
					{
						slot = ExecProcNode(outerPlanState(state));
					}
					else
					{
						/*
						 * We need to call CHECK_FOR_INTERRUPTS() here
						 * since if we are processing only from temp files,
						 * ExecProcNode is not called
						 */
						CHECK_FOR_INTERRUPTS();

						slot = state->extraSlot;
						tuplestore_gettupleslot(state->tempIn, true, slot);
					}

					if (TupIsNull(slot))
					{
						if (state->source == SS_OUTER)
							appendStringInfo(state->pass_info, "%lld", state->timestampIn);

						/*
						 * If we have read all tuples for the outer node
						 * switch to temp (FIXME: comment is wrong)
						 */
						if (state->source == SS_TEMP)
						{
							tuplestore_end(state->tempIn);
							state->tempIn = NULL;
						}

						if (state->timestampOut == 0)
						{
							/*
							 * We haven't written any tuples to the temp, so
							 * we are done.
							 */
							tuplestore_end(state->tempOut);
							state->tempOut = NULL;

							state->status = SS_DONE;
							return NULL;
						}
						else
						{
							state->pass++;
							appendStringInfo(state->pass_info, ", %lld", state->timestampOut);
							elog(DEBUG1, "start pass %lld with %lld tuples in temp", state->pass, state->timestampOut);

							state->source = SS_TEMP;
							state->tempIn = state->tempOut;

							/*
							 * By using work_mem = 0 we force the tuples in
							 * the tuplestore to go directly to the temp
							 * file
							 */
							state->tempOut = tuplestore_begin_heap(false, false, 0 /* work_mem */);

							state->timestampOut = 0;

							/*
							 * We just clean the window here, the tuples have
							 * allready been piped out.
							 */
							tuplewindow_clean(window);
						}
						break;
					}

					state->timestampIn++;

					tuplewindow_rewind(window);
					if (state->flags & SL_FLAGS_RANKED)
					{
						if (state->flags & SL_FLAGS_ENTROPY)
							tuplewindow_setinsertrank(window, ExecSkylineRank(state, slot));
						else if (state->flags & SL_FLAGS_RANDOM)
							tuplewindow_setinsertrank(window, ExecSkylineRandom());
						else
							Assert(0);
					}
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
								tuplewindow_puttupleslot(window, slot, 0, false);

								/*
								 * We can pipe out the tuple here.
								 */
								return slot;
							}
							else
							{
								tuplestore_puttupleslot(state->tempOut, slot);
								state->timestampOut++;
							}
							break;
						}

						tuplewindow_gettupleslot(window, inner_slot, false);

						cmp = ExecSkylineIsDominating(state, inner_slot, slot);

						/*
						 * The tuple in slot is dominated by a inner_slot in
						 * the window, so fetch the next.
						 */
						if (cmp == SKYLINE_CMP_FIRST_DOMINATES || (cmp == SKYLINE_CMP_ALL_EQ && node->skyline_distinct))
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
ExecSkyline(SkylineState *state)
{
	Skyline    *node = (Skyline *) state->ss.ps.plan;

	switch (state->skyline_method)
	{
		case SM_1DIM:
			return ExecSkyline_1Dim(state, node);
		case SM_1DIM_DISTINCT:
			return ExecSkyline_1DimDistinct(state, node);
		case SM_2DIM_PRESORT:
			return ExecSkyline_2DimPreSort(state, node);
		case SM_MATERIALIZEDNESTEDLOOP:
			return ExecSkyline_MaterializedNestedLoop(state, node);
		case SM_BLOCKNESTEDLOOP:
			return ExecSkyline_BlockNestedLoop(state, node);
		case SM_SFS:
			return ExecSkyline_SortFilterSkyline(state, node);
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
ExecEndSkyline(SkylineState *state)
{
	/*
	 * clean out the tuple table
	 */
	ExecClearTuple(state->ss.ss_ScanTupleSlot);
	ExecClearTuple(state->ss.ps.ps_ResultTupleSlot);
	if (ExecSkylineNeedExtraSlot((Skyline *)state->ss.ps.plan))
	{
		ExecClearTuple(state->extraSlot);
	}

	/*
	 * shut down the subplan
	 */
	ExecEndNode(outerPlanState(state));
}

/*
 * ExecReScanSkyline
 *
 *	FIXME
 */
void
ExecReScanSkyline(SkylineState *state, ExprContext *exprCtxt)
{
	/* FIXME: code coverage = 0 !!! */

	state->status = SS_INIT;

	/* must clear first tuple */
	ExecClearTuple(state->ss.ss_ScanTupleSlot);

	if (((PlanState *) state)->lefttree &&
		((PlanState *) state)->lefttree->chgParam == NULL)
		ExecReScan(((PlanState *) state)->lefttree, exprCtxt);

	state->status = SS_OUTER;
	state->pass = 1;

	state->cmps_tuples = 0;
	state->cmps_fields = 0;
	resetStringInfo(state->pass_info);

	/* reinit tuple window */
	ExecSkylineInitTupleWindow(state, (Skyline *) state->ss.ps.plan);


	/* close temp files */
	if (state->tempIn)
	{
		tuplestore_end(state->tempIn);
		state->tempIn = NULL;
	}

	if (state->tempOut)
	{
		tuplestore_end(state->tempOut);
		state->tempOut = NULL;
	}

	/* FIXME: there is maybe more that need to be done here */
}
