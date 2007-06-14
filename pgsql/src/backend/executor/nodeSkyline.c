#include "postgres.h"

#include "access/nbtree.h"
#include "executor/execdebug.h"
#include "executor/executor.h"
#include "executor/nodeSkyline.h"
#include "miscadmin.h"
#include "utils/datum.h"
#include "utils/tuplestore.h"
#include "utils/tuplewindow.h"
#include "utils/lsyscache.h"


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

#define SKYLINE_CMP_ALL_EQ 1
#define SKYLINE_CMP_FIRST_DOMINATES 2
#define SYKLINE_CMP_SECOND_DOMINATES 3
#define SKYLINE_CMP_INCOMPARABLE 4

int
ExecSkylineIsDominating(SkylineState *node, TupleTableSlot *inner_slot, TupleTableSlot *slot)
{
	Skyline	*sl = (Skyline*)node->ss.ps.plan;
	int		i;
	bool	cmp_all_eq = true;
	bool	cmp_lt = false;
	bool	cmp_gt = false;
	bool	cmp_diff_eq = true;
	
	for (i=0; i<sl->numCols; ++i) {
		Datum	datum1;
		Datum	datum2;
		bool	isnull1;
		bool	isnull2;
		int		attnum = sl->skylineColIdx[i];
		int		cmp;

		datum1 = slot_getattr(inner_slot, attnum, &isnull1);
		datum2 = slot_getattr(slot, attnum, &isnull2);

		cmp = inlineApplyCompareFunction(&(node->compareOpFn[i]), node->compareFlags[i], datum1, isnull1, datum2, isnull2);

		cmp_all_eq &= (cmp == 0);

		switch (sl->skylineByDir[i]) {
			case SKYLINEBY_DEFAULT:
			case SKYLINEBY_MIN:
			case SKYLINEBY_MAX:
			case SKYLINEBY_USING:
				if (cmp < 0)
					cmp_lt = true;
				else if (cmp > 0)
					cmp_gt = true;

				break;
			case SKYLINEBY_DIFF:
				cmp_diff_eq = cmp_diff_eq && (cmp == 0);

				break;
			default:
				elog(ERROR, "unrecognized skylineby_dir: %d", sl->skylineByDir[i]);
				break;
		}
	}

	if (cmp_all_eq)
		return SKYLINE_CMP_ALL_EQ;

	if (cmp_lt && !cmp_gt && cmp_diff_eq)
		return SKYLINE_CMP_FIRST_DOMINATES;

	if (!cmp_lt && cmp_gt && cmp_diff_eq)
		return SYKLINE_CMP_SECOND_DOMINATES;

	return SKYLINE_CMP_INCOMPARABLE;
}

static void
ExecSkylineGetOrderingOp(Skyline *sl, int idx, FmgrInfo *compareOpFn, int *compareFlags)
{
	/* some sanity checks */
	AssertArg(sl != NULL);
	AssertArg(0 <= idx && idx < sl->numCols);
	AssertArg(compareOpFn != NULL);
	AssertArg(compareFlags != NULL);

	{
		Oid		compareOperator = sl->skylinebyOperators[idx];
		Oid		compareFunction;
		bool	reverse;

		/* lookup the ordering function */
		// FIXME: fix wording for error message
		if (!get_compare_function_for_ordering_op(compareOperator, &compareFunction, &reverse))
			elog(ERROR, "operator %u is not a valid ordering operator",	compareOperator);

		fmgr_info(compareFunction, compareOpFn);

		/* set ordering flags */
		*compareFlags = reverse ? SK_BT_DESC : 0;
		if (sl->nullsFirst[idx])
			*compareFlags |= SK_BT_NULLS_FIRST;
	}
}

SkylineState *
ExecInitSkyline(Skyline *node, EState *estate, int eflags)
{
	SkylineState *slstate;

	/* check for unsupported flags */
	Assert(!(eflags & (EXEC_FLAG_BACKWARD | EXEC_FLAG_MARK)));

	slstate = makeNode(SkylineState);
	slstate->ss.ps.plan = (Plan *) node;
	slstate->ss.ps.state = estate;

	slstate->sl_done = false;

	/*
	 * Miscellaneous initialization
	 *
	 * Skyline nodes don't initialize their ExprContexts because they never call
	 * ExecQual or ExecProject.
	 */

#define SKYLINE_NSLOTS 2
#if 0
	/*
	 * create expression context
	 */
	ExecAssignExprContext(estate, &slstate->ss.ps);
#endif

	/*
	 * tuple table initialization
	 */
	ExecInitScanTupleSlot(estate, &slstate->ss);
	ExecInitResultTupleSlot(estate, &slstate->ss.ps);

	/* for extra slot */
	// slstate->sl_extraSlot = ExecInitExtraTupleSlot(estate);

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
#if 0
	// I think we can do so
	/*
	 * We shield the child node from the need to support REWIND, BACKWARD, or
	 * MARK/RESTORE.
	 */
	eflags &= ~(EXEC_FLAG_REWIND | EXEC_FLAG_BACKWARD | EXEC_FLAG_MARK);
#endif
	/* for testing we want the child node to support rewind and mark */
	eflags |= EXEC_FLAG_REWIND | EXEC_FLAG_BACKWARD | EXEC_FLAG_MARK;

	outerPlanState(slstate) = ExecInitNode(outerPlan(node), estate, eflags);

	/*
	 * initialize tuple type.  no need to initialize projection info because
	 * this node doesn't do projections.
	 */
	ExecAssignResultTypeFromTL(&slstate->ss.ps);
	ExecAssignScanTypeFromOuterPlan(&slstate->ss);
	/* for extra slot */
	// ExecSetSlotDescriptor(slstate->sl_extraSlot, ExecGetResultType(outerPlanState(slstate)));
	slstate->ss.ps.ps_ProjInfo = NULL;

	/* cache compare function info */
	{
		int i;

		slstate->compareOpFn = (FmgrInfo *) palloc(node->numCols * sizeof(FmgrInfo));
		slstate->compareFlags = (int *) palloc(node->numCols  * sizeof(int));

		for (i=0; i<node->numCols; ++i)
			ExecSkylineGetOrderingOp(node, i, &(slstate->compareOpFn[i]), &slstate->compareFlags[i]);
	}

	return slstate;
}

int
ExecCountSlotsSkyline(Skyline * node)
{
	return ExecCountSlotsNode(outerPlan(node)) + SKYLINE_NSLOTS;
}

TupleTableSlot *
ExecSkyline(SkylineState *node)
{
	Skyline *sl = (Skyline*)node->ss.ps.plan;

	if (sl->numCols == 1 && sl->skyline_distinct) {
		if (!node->sl_done) {
			int		compareFlags = node->compareFlags[0];
			FmgrInfo	compareOpFn = node->compareOpFn[0];
			Datum	datum1;
			Datum	datum2;
			bool	isnull1;
			bool	isnull2;
			int		attnum = sl->skylineColIdx[0];
			TupleTableSlot *resultSlot = node->ss.ps.ps_ResultTupleSlot;

			for (;;)
			{
				TupleTableSlot *slot = ExecProcNode(outerPlanState(node));

				if (TupIsNull(slot))
					break;

				if (TupIsNull(resultSlot)) {
					ExecCopySlot(resultSlot, slot);
					datum1 = slot_getattr(resultSlot, attnum, &isnull1);
				}
				else {
					datum2 = slot_getattr(slot, attnum, &isnull2);

					if (inlineApplyCompareFunction(&compareOpFn, compareFlags, datum1, isnull1, datum2, isnull2) > 0) {
						/* using the result slot avoids copying of varlen attrs */
						ExecCopySlot(resultSlot, slot);
						datum1 = slot_getattr(resultSlot, attnum, &isnull1);
					}
				}
			}

			node->sl_done = true;
			return resultSlot;
		}
		else {
			return NULL;
		}
	}
	else if (sl->numCols == 1 && !sl->skyline_distinct) {
		if (!node->sl_done) {
			int		compareFlags = node->compareFlags[0];
			FmgrInfo	compareOpFn = node->compareOpFn[0];
			Datum	datum1;
			Datum	datum2;
			bool	isnull1;
			bool	isnull2;
			bool	first = true;
			int		attnum = sl->skylineColIdx[0];
			TupleTableSlot *resultSlot = node->ss.ps.ps_ResultTupleSlot;
			int16	typlen;
			bool	typbyval;

			get_typlenbyval(resultSlot->tts_tupleDescriptor->attrs[sl->skylineColIdx[0]-1]->atttypid, &typlen, &typbyval);

			node->tuplestorestate = tuplestore_begin_heap(false, false, work_mem);
			tuplestore_set_eflags(node->tuplestorestate, EXEC_FLAG_MARK);
			for (;;)
			{
				TupleTableSlot *slot = ExecProcNode(outerPlanState(node));

				if (TupIsNull(slot))
					break;

				if (first) {
					datum1 = datumCopy(slot_getattr(slot, attnum, &isnull1), typbyval, typlen);
					tuplestore_puttupleslot(node->tuplestorestate, slot);

					first = false;
				}
				else {
					int cmp;
					datum2 = slot_getattr(slot, attnum, &isnull2);

					cmp = inlineApplyCompareFunction(&compareOpFn, compareFlags, datum1, isnull1, datum2, isnull2);

					if (cmp == 0) {
						tuplestore_puttupleslot(node->tuplestorestate, slot);
					}
					else if (cmp > 0) {
						if (DatumGetPointer(datum1) != NULL)
							datumFree(datum1, typbyval, typlen);
						datum1 = datumCopy(datum2, typbyval, typlen); isnull1 = isnull2;

						tuplestore_catchup(node->tuplestorestate);
						tuplestore_puttupleslot(node->tuplestorestate, slot);
					}
				}
			}

			if (DatumGetPointer(datum1) != NULL)
				datumFree(datum1, typbyval, typlen);

			node->sl_done = true;

			/* fall trough to the sl_done, to return the first tuple */
		}
	
		Assert(node->sl_done);

		/* pipe out the tuples from the tuplestore */
		Assert(node->tuplestorestate != NULL);

		if (tuplestore_gettupleslot(node->tuplestorestate, true, node->ss.ps.ps_ResultTupleSlot))
			return node->ss.ps.ps_ResultTupleSlot;
		else
			return NULL;
	}
	else {
		if (!node->sl_done) {
			TupleWindowState *window = node->window = tuplewindow_begin(work_mem);

			for (;;) {
				TupleTableSlot *slot = ExecProcNode(outerPlanState(node));
				TupleTableSlot *inner_slot = node->ss.ps.ps_ResultTupleSlot;

				if (TupIsNull(slot))
					break;

				tuplewindow_rewind(window);
				for (;;) {
					int cmp;

					if (tuplewindow_ateof(window)) {
						// slot is not dominated, put it in the window 
						// TODO: check for avilable space
						tuplewindow_puttupleslot(window, slot);
						break;
					}
				
					tuplewindow_gettupleslot(window, inner_slot);

					cmp = ExecSkylineIsDominating(node, inner_slot, slot);

					if (sl->skyline_distinct && cmp == SKYLINE_CMP_ALL_EQ)
						break;

					// slot is dominated by a inner_slot in the window, so fetch the next
					if (cmp == SKYLINE_CMP_FIRST_DOMINATES)
						break;
					
					if (cmp == SYKLINE_CMP_SECOND_DOMINATES)
						tuplewindow_removecurrent(window);

					tuplewindow_movenext(window);
				}
			}

			// tuplewindow_end(window);
			tuplewindow_rewind(window);
			node->sl_done = true;
		}

		{
			TupleWindowState *window = node->window;

			if (tuplewindow_ateof(window))
				return NULL;
			else {
				TupleTableSlot *resultSlot = node->ss.ps.ps_ResultTupleSlot;
				tuplewindow_gettupleslot(window, resultSlot);
				tuplewindow_movenext(window);
				return resultSlot;
			}
		}
	}

#ifdef NOT_USED
		TupleTableSlot *resultSlot = node->ss.ps.ps_ResultTupleSlot;

		if (!node->sl_done) {
			ExecMarkPos(outerPlanState(node));
			node->sl_pos = 0;
			node->sl_done = true;
		}

		Assert(node->sl_done);

		for (;;) {
			TupleTableSlot *slot;
			ItemPointerData resultPointer;
			int64 sl_innerpos = 0;

			ExecRestrPos(outerPlanState(node));
			slot = ExecProcNode(outerPlanState(node));
			node->sl_pos++;
			ExecMarkPos(outerPlanState(node));

			if (TupIsNull(slot))
				return NULL;

			ExecCopySlot(resultSlot, slot);
			ItemPointerCopy(&(slot->tts_tuple->t_self), &resultPointer);

			ExecReScan(outerPlanState(node), NULL);

			for (;;) {
				int cmp;
				TupleTableSlot *inner_slot = ExecProcNode(outerPlanState(node));
				sl_innerpos++;

				if (TupIsNull(inner_slot)) {
					/* the tuple in the resultSlot is not dominated, return it */
					return resultSlot;
				}

				/* is inner_slot dominating resultSlot? */
				cmp = ExecSkylineIsDominating(node, inner_slot, resultSlot);

				if (sl->skyline_distinct) {
					if (cmp == SKYLINE_CMP_ALL_EQ) {
						/* the inner tuple is before the result tuple, so don't output the result tuple */
						if (sl_innerpos < node->sl_pos)
							break;
					}
				}
					
				if (cmp == SKYLINE_CMP_FIRST_DOMINATES)
					break;
			}
		}
	}
	#endif
}


void
ExecEndSkyline(SkylineState *node)
{
	SO1_printf("ExecEndSkyline: %s\n",
			   "shutting down skyline node");

	/* free tuple store state if allocated */
	if (node->tuplestorestate) {
		tuplestore_end((Tuplestorestate*)node->tuplestorestate);
		node->tuplestorestate = NULL;
	}

#if 0
	ExecFreeExprContext(&node->ss.ps);
#endif

	/*
	 * clean out the tuple table
	 */
	ExecClearTuple(node->ss.ss_ScanTupleSlot);
	/* must drop pointer to sort result tuple */
	ExecClearTuple(node->ss.ps.ps_ResultTupleSlot);

	/*
	 * shut down the subplan
	 */
	ExecEndNode(outerPlanState(node));

	SO1_printf("ExecEndSkyline: %s\n",
			   "skyline node shutdown");
}
