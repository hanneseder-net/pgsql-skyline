/*-------------------------------------------------------------------------
 *
 * tuplewndheap.c
 *    FIXME
 *
 * Portions Copyright (c) 2007, PostgreSQL Global Development Group
 *
 *
 * DESCRIPTION
 *    FIXME
 *
 *
 * IDENTIFICATION
 *	  $PostgreSQL: $
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"

#include "utils/tuplewndheap.h"
#include "utils/memutils.h"

/* FIXME: This is from tuplestore.c and could be factored out. */
#define LACKMEM(state)		((state)->availMem < 0)
#define USEMEM(state,amt)	((state)->availMem -= (amt))
#define FREEMEM(state,amt)	((state)->availMem += (amt))

typedef struct TupleWindowSlot
{
	void	   *tuple;
	double		rank;
} TupleWindowSlot;

struct TupleWndHeapState
{
	long		availMem;
	int			availSlots;		/* if -1 only restrict mem */

	int			maxKBytes;
	int			maxSlots;

	int			count;
	int			current;

	TupleWindowSlot	   *heap;
};

/*
 * tuplewndheap_begin
 *
 *	Initialize a tuplewndheap. If maxSlots is unequal to -1 then the tuple-
 *	window is only restricted by the number of slots, memory is not
 *	considered in that case. If maxSlots is equal to -1 maxKBytes is the
 *	limiting factor.
 */
TupleWndHeapState *
tuplewndheap_begin(int maxKBytes, int maxSlots)
{
	TupleWndHeapState *state;

	state = (TupleWndHeapState *) palloc0(sizeof(TupleWndHeapState));

	state->maxKBytes = maxKBytes;
	state->maxSlots = maxSlots;

	state->availMem = maxKBytes * 1024L;
	state->availSlots = maxSlots;

	/*
	 * Create heap, we don't count this memory usage.
	 */
	Assert(maxSlots > 0); /* FIXME */
	state->heap = (TupleWindowSlot *) palloc0(sizeof(TupleWindowSlot) * maxSlots);

	/*
	 * Set cursor (current) to sentinel.
	 */
	state->current = 0;

	return state;
}

/*
 * tuplewndheap_has_freespace
 *
 *	Returns true, if the tuple window is large enough to store at least
 *	one more tuple.
 */
bool
tuplewndheap_has_freespace(TupleWndHeapState *state)
{
	if (state->availSlots != -1)
		return (state->availSlots > 0);
	else
		return !LACKMEM(state);
}

/*
 * tuplewndheap_puttupleslot
 *
 *	Adds the tuple in slot to the tuplewndheap.
 */
void
tuplewndheap_puttupleslot(TupleWndHeapState *state,
						 TupleTableSlot *slot,
						 double rank)
{
	MinimalTuple tuple;

	/*
	 * Form a MinimalTuple in working memory.
	 */
	tuple = ExecCopySlotMinimalTuple(slot);
	USEMEM(state, GetMemoryChunkSpace(tuple));

	/* FIXME: */
	if (state->availSlots != -1)
	{
		/*
		 * If we are counting the slots, then at this point availSlots must be
		 * > 0.
		 */
		AssertState(state->availSlots > 0);

		state->availSlots--;
	}

	state->heap[state->count].tuple = tuple;
	state->heap[state->count].rank = rank;

	++state->count;

	/* FIXME: maintain heap conditions */
}

/*
 * tuplewndheap_removeslot
 *
 *	Helper for tuplewndheap_removecurrent.
 */
static void
tuplewndheap_removeslot(TupleWndHeapState *state,
					   int slot,
					   bool freetuple)
{
	AssertArg(state != NULL);
	AssertArg(0 <= slot && slot < state->count);
	Assert(slot != state->nil);

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
	 * (current) to the next slot. So don't call tuplewndheap_movenext in that
	 * case.
	 */
	if (state->current == slot)
		state->current = next;
}

/*
 * tuplewndheap_rewind
 *
 *	Resets the cursor (current) to the first tuple in the window.
 */
void
tuplewndheap_rewind(TupleWndHeapState *state)
{
	AssertArg(state != NULL);

	state->current = 0;
}

/*
 * tuplewndheap_ateof
 *
 *	Returns true if the cursor (current) is at the end of the tuplewndheap.
 */
bool
tuplewndheap_ateof(TupleWndHeapState *state)
{
	AssertArg(state != NULL);

	return (state->current < state->count);
}

/*
 * tuplewndheap_movenext
 *
 *	Move the cursor (current) to the next slot, also so
 *	tuplewndheap_removecurrent, which also advances the
 *	cursor. Use tuplewndheap_rewind to reset the cursor to
 *	the beginning.
 */
void
tuplewndheap_movenext(TupleWndHeapState *state)
{
	AssertArg(state != NULL);

	if (state->current < state->count)
		++state->current;
}

/*
 * tuplewndheap_gettupleslot
 *
 *	FIXME
 */
bool
tuplewndheap_gettupleslot(TupleWndHeapState *state,
						 TupleTableSlot *slot,
						 bool removeit)
{
	AssertArg(state != NULL);
	AssertArg(slot != NULL);

	if (state->current < state->count)
	{
		if (removeit)
		{
			/*
			 * Remove the tuple from the tuple window, but don't free the the
			 * tuple, let the ExecStoreMinimalTuple pfree it.
			 */
			ExecStoreMinimalTuple(state->heap[state->current].tuple, slot, true);
			tuplewndheap_removeslot(state, state->current, false);
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
 * tuplewndheap_removecurrent
 *
 *	Removes the current slot from the window and moves the
 *	cursor (current) to the next slot. No extra call to tuplewndheap_movenext
 *	needed in that case.
 */
void
tuplewndheap_removecurrent(TupleWndHeapState *state)
{
	AssertArg(state != NULL);
	Assert(state->current != NULL);
	Assert(state->current != state->nil);
	tuplewndheap_removeslot(state, state->current, true);
}

/*
 * tuplewndheap_clean
 * 
 *	Removes all tuple from the window. tuplewndheap_removeall might be a
 *	better name, but anyway it's a tuple*window* and windows are cleaned.
 */
void
tuplewndheap_clean(TupleWndHeapState *state)
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
 * tuplewndheap_end
 *
 *	Release resources and cleanup.
 */
void
tuplewndheap_end(TupleWndHeapState *state)
{
	AssertArg(state != NULL);

	/* free all slots */
	tuplewndheap_clean(state);

	/* free the sentinel */
	pfree(state->nil);

	/* free the state buffer */
	pfree(state);
}
