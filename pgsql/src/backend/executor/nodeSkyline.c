#include "postgres.h"

#include "access/nbtree.h"
#include "executor/execdebug.h"
#include "executor/executor.h"
#include "executor/nodeSkyline.h"
#include "utils/tuplestore.h"
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

int
ExecSkylineIsDominating(SkylineState *node, TupleTableSlot *inner_slot, TupleTableSlot *slot)
{
	Skyline	*sl = (Skyline*)node->ss.ps.plan;
	int		i;
	bool	cmp_lt = false;
	bool	cmp_gt = false;

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
		if (cmp < 0)
			cmp_lt = true;
		else if (cmp > 0)
			cmp_gt = true;
	}

	if (cmp_lt && !cmp_gt)
		return -1;
	else if (!cmp_lt && cmp_gt)
		return 1;
	else
		return 0; /* it's incompare able */
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

	/* FIXME: the 1d cases do not handle by-ref-types correctly */
	/* FIXME handle by reference types correctly, see tuplesort_putdatum, datumCopy */
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
					datum1 = slot_getattr(slot, attnum, &isnull1);
					ExecCopySlot(resultSlot, slot);
				}
				else {
					datum2 = slot_getattr(slot, attnum, &isnull2);

					if (inlineApplyCompareFunction(&compareOpFn, compareFlags, datum1, isnull1, datum2, isnull2) > 0) {
						datum1 = datum2; isnull1 = isnull2;
						ExecCopySlot(resultSlot, slot);
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
#ifdef NOT_INCLUDED	
	else if (sl->numCols == 1 && !sl->skyline_distinct) {
		/* FIXME handle by reference types correctly, see tuplesort_putdatum, datumCopy */
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

			node->tuplestorestate = tuplestore_begin_heap(false, false, 1024 /* FIXME maxKBytes */);
			tuplestore_set_eflags(node->tuplestorestate, EXEC_FLAG_MARK);
			for (;;)
			{
				TupleTableSlot *slot = ExecProcNode(outerPlanState(node));

				if (TupIsNull(slot))
					break;

				if (first) {
					datum1 = slot_getattr(slot, attnum, &isnull1);
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
						datum1 = datum2; isnull1 = isnull2;

						tuplestore_catchup(node->tuplestorestate);
						tuplestore_puttupleslot(node->tuplestorestate, slot);
					}
				}
			}

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
#endif
	else {
		TupleTableSlot *resultSlot = node->ss.ps.ps_ResultTupleSlot;

		if (!node->sl_done) {
			ExecMarkPos(outerPlanState(node));
			node->sl_done = true;
		}

		Assert(node->sl_done);

		for (;;) {
			TupleTableSlot *slot;

			ExecRestrPos(outerPlanState(node));
			slot = ExecProcNode(outerPlanState(node));
			ExecMarkPos(outerPlanState(node));

			if (TupIsNull(slot))
				return NULL;

			ExecCopySlot(resultSlot, slot);

			ExecReScan(outerPlanState(node), NULL);

			for (;;) {
				TupleTableSlot *inner_slot = ExecProcNode(outerPlanState(node));
				int cmp;

				if (TupIsNull(inner_slot)) {
					/* the tuple in the resultSlot is not dominated, return it */
					return resultSlot;
				}

				/* is inner_slot dominating resultSlot? */

				cmp = ExecSkylineIsDominating(node, inner_slot, resultSlot);
				if (cmp < 0) {
					break;
				}
			}
		}
	}
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
