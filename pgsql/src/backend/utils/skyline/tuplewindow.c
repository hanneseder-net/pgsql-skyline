#include "postgres.h"

#include "utils/tuplewindow.h"
#include "utils/memutils.h"

/* this is from tuplestore.c and could be factored out */
#define LACKMEM(state)		((state)->availMem < 0)
#define USEMEM(state,amt)	((state)->availMem -= (amt))
#define FREEMEM(state,amt)	((state)->availMem += (amt))

typedef struct TupleWindowSlot {
	void   *tuple;
	int64	timestamp;
	struct TupleWindowSlot	*next;
	struct TupleWindowSlot	*prev;
} TupleWindowSlot;

struct TupleWindowState {
	long		availMem;		
	long		availSlots;				// if -1 only restrict mem
	TupleWindowSlot	*nil;
	TupleWindowSlot *current;
};


/*
 *  We are using a double linked list with a sentinel
 */
TupleWindowState *
tuplewindow_begin(int maxKBytes, int maxSlots)
{
	TupleWindowState *state;

	state = (TupleWindowState *) palloc0(sizeof(TupleWindowState));

	state->availMem = maxKBytes * 1024L;
	state->availSlots = maxSlots;

	/* create sentinel, we don't count this memory usage */
	state->nil = (TupleWindowSlot *) palloc(sizeof(TupleWindowSlot));
	state->nil->next = state->nil;
	state->nil->prev = state->nil;

	state->current = state->nil;

	return state;
}

bool
tuplewindow_has_freespace(TupleWindowState *state)
{
	if (state->availSlots != -1)
		return (state->availSlots > 0);
	else 
		return !LACKMEM(state);
}

void
tuplewindow_puttupleslot(TupleWindowState *state, 
						 TupleTableSlot *slot,
						 int64 timestamp)
{
	MinimalTuple tuple;
	TupleWindowSlot *windowSlot;

	/*
	 * Form a MinimalTuple in working memory
	 */
	tuple = ExecCopySlotMinimalTuple(slot);
	USEMEM(state, GetMemoryChunkSpace(tuple));

	windowSlot = (TupleWindowSlot*)palloc(sizeof(TupleWindowSlot));
	USEMEM(state, GetMemoryChunkSpace(windowSlot));

	if (state->availSlots != -1)
	{
		/* if we are counting the slots, then at this point availSlots 
		 * must be > 0
		 */
		AssertState(state->availSlots > 0);

		state->availSlots--;
	}

	windowSlot->tuple = tuple;
	windowSlot->timestamp = timestamp;

	/* we insert at the end of the list, to preserve
     * relative tuple order
	 */
	windowSlot->next = state->nil;
	windowSlot->prev = state->nil->prev;
	state->nil->prev = windowSlot;
	windowSlot->prev->next = windowSlot;
}

static void
tuplewindow_removeslot(TupleWindowState *state,
					   TupleWindowSlot *slot,
					   bool freetuple)
{
	TupleWindowSlot *next = slot->next;

	AssertArg(state != NULL);
	AssertArg(slot != NULL);
	Assert(slot != state->nil);

	/* remove from linked list */
	slot->prev->next = slot->next;
	slot->next->prev = slot->prev;

	/* invalidate slot, in production code this is not needed */
	slot->prev = NULL;
	slot->next = NULL;

	/* HACK: we reclame the space, because ExecStoreMinimalTuple called with
	 * true, so it will free the tuple
	 */
	FREEMEM(state, GetMemoryChunkSpace(slot->tuple));
	if (freetuple) {
		pfree(slot->tuple);
	}

	FREEMEM(state, GetMemoryChunkSpace(slot));
	pfree(slot);

	if (state->availSlots != -1) {
		/* if we are restricting the slots, then one more becomes available here */
		state->availSlots++;
	}

	/* NOTE: if we are removing the current slot, reposition the cursor (current) to
	 * the next slot
	 */
	if (state->current == slot)
		state->current = next;
}

void
tuplewindow_rewind(TupleWindowState *state)
{
	AssertArg(state != NULL);

	state->current = state->nil->next;
}

bool
tuplewindow_ateof(TupleWindowState *state)
{
	AssertArg(state != NULL);

	return (state->current == state->nil);
}

void
tuplewindow_movenext(TupleWindowState *state)
{
	AssertArg(state != NULL);

	if (state->current != state->nil)
		state->current = state->current->next;
}

int64
tuplewindow_timestampcurrent(TupleWindowState *state)
{
	AssertArg(state != NULL);
	Assert(state->current != NULL);
	Assert(state->current != state->nil);

	return state->current->timestamp;
}

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
			/* remove the tuple from the tuple window, but don't free the
			 * the tuple, let the ExecStoreMinimalTuple pfree it, HM? FIXME
			 */
			ExecStoreMinimalTuple(state->current->tuple, slot, true);
			tuplewindow_removeslot(state, state->current, false);
		}
		else
		{
			/* leave the tuple in the window */
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
 *  Removes the current slot from the window and moves the
 *  cursor (current) to the next slot
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
 * tuplewindow_end
 *
 *  Release resources and cleanup
 */
void
tuplewindow_end(TupleWindowState *state)
{
	/*
	 * NOTE: we could also use the external interface, if we want to be
	 * more independend from the internal representation
	 */
	/*
	tuplewindow_rewind(state);
	while (!tuplewindow_ateof(state))
	{
		tuplewindow_removecurrent(state);
	}
	*/

	/* free all slots */
	TupleWindowSlot *slot = state->nil->next;

	while (slot != state->nil) {
		TupleWindowSlot *next = slot->next;
		pfree(slot->tuple);
		pfree(slot);
		slot = next;
	}

	/* free the sentinel */
	pfree(state->nil);

	/* free the state buffer */
	pfree(state);
}
