/*-------------------------------------------------------------------------
 *
 * tuplewindow.c
 *    Routines to handle maintain a tuplewindow, used for skyline nodes.
 *    see: executor/nodeSkyline.c
 *
 * Portions Copyright (c) 2007, PostgreSQL Global Development Group
 *
 *
 * DESCRIPTION
 *    We are using a double linked list with a sentinel.
 *
 *    For double linked list with sentinel see: [COR2002, Ch. 10.2, p 204--209]
 *    [COR2002] Cormen, Thomas H., Introduction to Algorithms,
 *    Second Edition, Third printing 2002, MIT Press, ISBN 0-262-03293-7
 *
 * IDENTIFICATION
 *	  $PostgreSQL: $
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"

#include <float.h>

#include "utils/tuplewindow.h"
#include "utils/memutils.h"

/* FIXME: This is from tuplestore.c and could be factored out. */
#define LACKMEM(state)		((state)->availMem < 0)
#define USEMEM(state,amt)	((state)->availMem -= (amt))
#define FREEMEM(state,amt)	((state)->availMem += (amt))

typedef struct TupleWindowSlot
{
	void	   *tuple;
	int64		timestamp;
	double		rank;
	struct TupleWindowSlot *next;
	struct TupleWindowSlot *prev;
} TupleWindowSlot;

struct TupleWindowState
{
	long		availMem;
	int			availSlots;		/* if -1 only restrict mem */

	int			maxKBytes;
	int			maxSlots;

	TupleWindowPolicy	policy;

	TupleWindowSlot *nil;
	TupleWindowSlot *current;

	double		insertrank;
	TupleWindowSlot *rankinsert;
};

/* forward decl's */
static void tuplewindow_puttupleslot_impl(TupleWindowState *state, TupleTableSlot *slot, int64 timestamp, TupleWindowSlot *insertbefore, double rank);

/*
 * tuplewindow_begin
 *
 *	Initialize a tuplewindow. If maxSlots is unequal to -1 then the tuple-
 *	window is only restricted by the number of slots, memory is not
 *	considered in that case. If maxSlots is equal to -1 maxKBytes is the
 *	limiting factor.
 */
TupleWindowState *
tuplewindow_begin(int maxKBytes, int maxSlots, TupleWindowPolicy policy)
{
	TupleWindowState *state;

	state = (TupleWindowState *) palloc0(sizeof(TupleWindowState));

	state->maxKBytes = maxKBytes;
	state->maxSlots = maxSlots;

	state->availMem = maxKBytes * 1024L;
	state->availSlots = maxSlots;

	/*
	 * NOTE: currently only these policies are suppored
	 */
	AssertArg(   (policy == TUP_WIN_POLICY_APPEND)
			  || (policy == TUP_WIN_POLICY_PREPEND)
		      || (policy == TUP_WIN_POLICY_RANKED)
			  );

	state->policy = policy;

	/*
	 * Create sentinel, we don't count this memory usage.
	 */
	state->nil = (TupleWindowSlot *) palloc(sizeof(TupleWindowSlot));
	state->nil->next = state->nil;
	state->nil->prev = state->nil;
	state->nil->timestamp = 0;
	state->nil->rank = DBL_MIN;

	/*
	 * Set cursor (current) to sentinel.
	 */
	state->current = state->nil;

	/*
	 * Set rank cursor
	 */
	state->insertrank = DBL_MIN;
	state->rankinsert = state->nil;

	return state;
}

/*
 * tuplewindow_has_freespace
 *
 *	Returns true, if the tuple window is large enough to store at least
 *	one more tuple.
 */
bool
tuplewindow_has_freespace(TupleWindowState *state)
{
	if (state->availSlots != -1)
		return (state->availSlots > 0);
	else
		return !LACKMEM(state);
}

/*
 * tuplewindow_removeslot
 *
 *	Helper for tuplewindow_removecurrent.
 */
static void
tuplewindow_removeslot(TupleWindowState *state,
					   TupleWindowSlot *slot,
					   bool freetuple)
{
	TupleWindowSlot *next;

	AssertArg(state != NULL);
	AssertArg(slot != NULL);
	Assert(slot != state->nil);

	next = slot->next;

	/* remove from linked list */
	slot->prev->next = slot->next;
	slot->next->prev = slot->prev;

	/* invalidate slot, in production code this is not needed */
	slot->prev = NULL;
	slot->next = NULL;

	/*
	 * HACK: We reclame the space here, because ExecStoreMinimalTuple called
	 * with true, so it will free the tuple later.
	 */
	FREEMEM(state, GetMemoryChunkSpace(slot->tuple));
	if (freetuple)
	{
		pfree(slot->tuple);
	}

	FREEMEM(state, GetMemoryChunkSpace(slot));
	pfree(slot);

	if (state->availSlots != -1)
	{
		/*
		 * If we are restricting the slots, then one more becomes available
		 * here.
		 */
		state->availSlots++;
	}

	/*
	 * NOTE: If we are removing the current slot, reposition the cursor
	 * (current) to the next slot. So don't call tuplewindow_movenext in that
	 * case.
	 */
	if (state->current == slot)
		state->current = next;

	if (state->rankinsert == slot)
		state->rankinsert = next;
}

/*
 * tuplewindow_rewind
 *
 *	Resets the cursor (current) to the first tuple in the window.
 */
void
tuplewindow_rewind(TupleWindowState *state)
{
	AssertArg(state != NULL);

	state->current = state->nil->next;
	state->rankinsert = state->nil->next;
}

/*
 * tuplewindow_ateof
 *
 *	Returns true if the cursor (current) is at the end of the tuplewindow.
 */
bool
tuplewindow_ateof(TupleWindowState *state)
{
	AssertArg(state != NULL);

	return (state->current == state->nil);
}

/*
 * tuplewindow_movenext
 *
 *	Move the cursor (current) to the next slot, also so
 *	tuplewindow_removecurrent, which also advances the
 *	cursor. Use tuplewindow_rewind to reset the cursor to
 *	the beginning.
 */
void
tuplewindow_movenext(TupleWindowState *state)
{
	AssertArg(state != NULL);

	if (state->current != state->nil)
	{
		if (state->policy == TUP_WIN_POLICY_RANKED 
			&& state->current == state->rankinsert)
		{
			if (state->current->rank <= state->insertrank)
				state->rankinsert = state->current->next;
		}

		state->current = state->current->next;
	}
}

/*
 * tuplewindow_timestampcurrent
 *
 *	Returns the timestamp of the current tuple.
 */
int64
tuplewindow_timestampcurrent(TupleWindowState *state)
{
	AssertArg(state != NULL);
	Assert(state->current != NULL);
	Assert(state->current != state->nil);

	return state->current->timestamp;
}

/*
 * tuplewindow_gettupleslot
 *
 *	FIXME
 */
bool
tuplewindow_gettupleslot(TupleWindowState *state,
						 TupleTableSlot *slot,
						 bool removeit)
{
	AssertArg(state != NULL);
	AssertArg(slot != NULL);

	if (state->current != state->nil)
	{
		if (removeit)
		{
			/*
			 * Remove the tuple from the tuple window, but don't free the the
			 * tuple, let the ExecStoreMinimalTuple pfree it.
			 */
			ExecStoreMinimalTuple(state->current->tuple, slot, true);
			tuplewindow_removeslot(state, state->current, false);
		}
		else
		{
			/* Leave the tuple in the window. */
			ExecStoreMinimalTuple(state->current->tuple, slot, false);
		}
		return true;
	}
	else
	{
		ExecClearTuple(slot);
		return false;
	}
}

/*
 * tuplewindow_removecurrent
 *
 *	Removes the current slot from the window and moves the
 *	cursor (current) to the next slot. No extra call to tuplewindow_movenext
 *	needed in that case.
 */
void
tuplewindow_removecurrent(TupleWindowState *state)
{
	AssertArg(state != NULL);
	Assert(state->current != NULL);
	Assert(state->current != state->nil);
	tuplewindow_removeslot(state, state->current, true);
}

/*
 * tuplewindow_clean
 * 
 *	Removes all tuple from the window. tuplewindow_removeall might be a
 *	better name, but anyway it's a tuple*window* and windows are cleaned.
 */
void
tuplewindow_clean(TupleWindowState *state)
{
	TupleWindowSlot *slot;

	AssertArg(state != NULL);

	/*
	 * Free all slots and the tuples in the slot.
	 */
	slot = state->nil->next;

	while (slot != state->nil)
	{
		TupleWindowSlot *next = slot->next;

		pfree(slot->tuple);
		pfree(slot);
		slot = next;
	}

	/*
	 * Reset sentinel.
	 */
	state->nil->next = state->nil;
	state->nil->prev = state->nil;

	/*
	 * Reset cursor (current) to sentinel.
	 */
	state->current = state->nil;

	/*
	 * We have free'd all slots and all tuples, so all the memory is
	 * available again.
	 */
	state->availMem = state->maxKBytes * 1024L;
	state->availSlots = state->maxSlots;
}

/*
 * tuplewindow_end
 *
 *	Release resources and cleanup.
 */
void
tuplewindow_end(TupleWindowState *state)
{
	AssertArg(state != NULL);

	/* free all slots */
	tuplewindow_clean(state);

	/* free the sentinel */
	pfree(state->nil);

	/* free the state buffer */
	pfree(state);
}

/*
 * tuplewindow_setinsertrank
 *
 * FIXME
 */
void
tuplewindow_setinsertrank(TupleWindowState *state, double rank)
{
	AssertArg(state != NULL);
	
	/*
	 * Setting the rank is only useful when using RANKED policy
	 */
	AssertState(state->policy == TUP_WIN_POLICY_RANKED);

	/*
	 * When setting the rank the cursor (current) has to be
	 * rewinded
	 */
	AssertState(state->current == state->nil->next);

	state->insertrank = rank;
}

/*
 * tuplewindow_puttupleslot_impl
 *
 *	The workhorse for tuplewindow_puttupleslot.
 */
static void
tuplewindow_puttupleslot_impl(TupleWindowState *state, TupleTableSlot *slot, int64 timestamp, TupleWindowSlot *insertbefore, double rank)
{
	MinimalTuple tuple;
	TupleWindowSlot *windowSlot;

	AssertState(state != NULL);

	/*
	 * Form a MinimalTuple in working memory.
	 */
	tuple = ExecCopySlotMinimalTuple(slot);
	USEMEM(state, GetMemoryChunkSpace(tuple));

	windowSlot = (TupleWindowSlot *) palloc(sizeof(TupleWindowSlot));
	USEMEM(state, GetMemoryChunkSpace(windowSlot));

	if (state->availSlots != -1)
	{
		/*
		 * If we are counting the slots, then at this point availSlots must be
		 * > 0.
		 */
		AssertState(state->availSlots > 0);

		state->availSlots--;
	}

	windowSlot->tuple = tuple;
	windowSlot->timestamp = timestamp;
	windowSlot->rank = rank;


	insertbefore->prev->next = windowSlot;
	windowSlot->prev = insertbefore->prev;
	windowSlot->next = insertbefore;
	insertbefore->prev = windowSlot;
}

/*
 * tuplewindow_puttupleslot
 *
 *	Adds the tuple in slot to the tuplewindow.
 *
 *  FIXME: on a forced insert we could return the removed tuple to the caller
 *  so it can be written to a temp file
 */
void
tuplewindow_puttupleslot(TupleWindowState *state,
						 TupleTableSlot *slot,
						 int64 timestamp,
						 bool forced)
{
	AssertArg(state != NULL);
	AssertArg(slot != NULL);

	switch (state->policy)
	{
	case TUP_WIN_POLICY_APPEND:
		if (forced)
		{
			/*
			 * We only insert if there is still space in the window.
			 */
			if (tuplewindow_has_freespace(state))
				tuplewindow_puttupleslot_impl(state, slot, timestamp, state->nil, DBL_MIN);
		}
		else
		{
			/*
			 * We insert at the end of the list, to preserve relative tuple order in
			 * the tuple window.
			 */
			AssertState(tuplewindow_has_freespace(state));
			tuplewindow_puttupleslot_impl(state, slot, timestamp, state->nil, DBL_MIN);
		}
		break;

	case TUP_WIN_POLICY_PREPEND:
		if (forced)
		{
			/*
			 * We only insert if there is still space in the window.
			 */
			if (tuplewindow_has_freespace(state))
				tuplewindow_puttupleslot_impl(state, slot, timestamp, state->nil->next, DBL_MIN);
		}
		else
		{
			/*
			 * We insert at the begining of the list.
			 */
			AssertState(tuplewindow_has_freespace(state));
			tuplewindow_puttupleslot_impl(state, slot, timestamp, state->nil->next, DBL_MIN);
		}
		break;

	case TUP_WIN_POLICY_RANKED:
		if (forced && !tuplewindow_has_freespace(state))
		{
			/*
			 * remove the slot with the lowest ranking
			 */
			Assert(state->nil->prev != state->nil);

			tuplewindow_removeslot(state, state->nil->prev, true);
		}

		/*
		 * We insert the tuple at the ranked position, this keeps the window
		 * ordered by the rank.
		 */
		AssertState(tuplewindow_has_freespace(state));
		tuplewindow_puttupleslot_impl(state, slot, timestamp, state->rankinsert, state->insertrank);
		break;

	default:
		elog(ERROR, "invalid tuple window policy: %d", state->policy);
		break;
	}
}
