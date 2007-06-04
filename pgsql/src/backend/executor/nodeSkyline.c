#include "postgres.h"

#include "access/nbtree.h"
#include "executor/execdebug.h"
#include "executor/executor.h"
#include "executor/nodeSkyline.h"
#include "utils/lsyscache.h"


/*
 * Inline-able copy of FunctionCall2() to save some cycles in sorting.
 */
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

	return slstate;
}

int
ExecCountSlotsSkyline(Skyline * node)
{
	return ExecCountSlotsNode(outerPlan(node)) + SKYLINE_NSLOTS;
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

TupleTableSlot *
ExecSkyline(SkylineState *node)
{
	Skyline *sl = (Skyline*)node->ss.ps.plan;

	if (sl->numCols == 1) {
		if (!node->sl_done) {
			int		compareFlags;
			FmgrInfo	compareOpFn;
			Datum	datum1;
			Datum	datum2;
			bool	isnull1;
			bool	isnull2;
			int		attnum = sl->skylineColIdx[0];
			TupleTableSlot *resultSlot = node->ss.ps.ps_ResultTupleSlot;

			ExecSkylineGetOrderingOp(sl, 0, &compareOpFn, &compareFlags);

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
	else {
		// pipe the trough
		TupleTableSlot *slot = ExecProcNode(outerPlanState(node));
		return slot;
	}

	/*
	if (slot != NULL) {
		TupleDesc tdesc = slot->tts_tupleDescriptor; 
	}
	*/

	/*
	// print_slot(slot);
	printf("--ColIdx->");
	for (i=0; i<(sl->numCols); i++) {
		printf("%d ", sl->skylineColIdx[i]);
	}
	printf("\n");
	*/
}

void
ExecEndSkyline(SkylineState *node)
{
	SO1_printf("ExecEndSkyline: %s\n",
			   "shutting down skyline node");
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
