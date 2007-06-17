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
	TupleWindowSlot	*head;
	TupleWindowSlot *current;
};

TupleWindowState *
tuplewindow_begin(int maxKBytes)
{
	TupleWindowState *state;

	state = (TupleWindowState *) palloc0(sizeof(TupleWindowState));

	state->availMem = maxKBytes * 1024L;

	return state;
}

bool
tuplewindow_has_freespace(TupleWindowState *state)
{
	return !LACKMEM(state);
	// for a one slot window
	// return (state->head == NULL);
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

	windowSlot->next = state->head;
	windowSlot->prev = NULL;
	windowSlot->tuple = tuple;
	windowSlot->timestamp = timestamp;

	if (state->head)
		state->head->prev = windowSlot;

	state->head = windowSlot;
	state->current = windowSlot;
}

static void
tuplewindow_removeslot(TupleWindowState *state,
					   TupleWindowSlot *slot,
					   bool freetuple)
{
	TupleWindowSlot *prev = slot->prev;
	TupleWindowSlot *next = slot->next;

	slot->prev = NULL;
	slot->next = NULL;

	if (slot == state->head) {
		state->head = next;
		if (next)
			next->prev = NULL;
	}
	else {
		Assert(prev != NULL);
		prev->next = next;
		if (next)
			next->prev = prev;
	}
	
	/* HACK: we re clame the space, because ExecStoreMinimalTuple called with
	 * true, so it will free the tuple
	 */
	FREEMEM(state, GetMemoryChunkSpace(slot->tuple));
	if (freetuple) {
		pfree(slot->tuple);
	}

	FREEMEM(state, GetMemoryChunkSpace(slot));
	pfree(slot);

	if (state->current == slot)
		state->current = next;
}

void
tuplewindow_rewind(TupleWindowState *state)
{
	state->current = state->head;
}

bool
tuplewindow_ateof(TupleWindowState *state)
{
	return (state->current == NULL);
}

void
tuplewindow_movenext(TupleWindowState *state)
{
	if (state->current)
		state->current = state->current->next;
}

int64
tuplewindow_timestampcurrent(TupleWindowState *state)
{
	Assert(state->current != NULL);
	return state->current->timestamp;
}

bool
tuplewindow_gettupleslot(TupleWindowState *state,
						 TupleTableSlot *slot,
						 bool removeit)
{
	if (state->current != NULL)	{
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
	else {
		ExecClearTuple(slot);
		return false;
	}
}

void
tuplewindow_removecurrent(TupleWindowState *state)
{
	TupleWindowSlot *next = NULL;

	Assert(state->current);

	next = state->current->next;
	tuplewindow_removeslot(state, state->current, true);
	state->current = next;
}

void
tuplewindow_end(TupleWindowState *state)
{
	TupleWindowSlot *head = state->head;

	while (head != NULL) {
		TupleWindowSlot *next = head->next;
		pfree(head->tuple);
		pfree(head);
		head = next;
	}
}
