#ifndef TUPLEWINDOW_H
#define TUPLEWINDOW_H

#include "executor/tuptable.h"

typedef struct TupleWindowState TupleWindowState;

typedef struct TupleWindowSlot {
	void   *tuple;
	struct TupleWindowSlot	*next;
	struct TupleWindowSlot	*prev;
} TupleWindowSlot;

extern TupleWindowState* tuplewindow_begin(int maxKBytes);
extern bool tuplewindow_has_freespace(TupleWindowState *state);
extern void tuplewindow_puttupleslot(TupleWindowState *state, TupleTableSlot *slot);
extern void tuplewindow_rewind(TupleWindowState *state);
extern bool tuplewindow_ateof(TupleWindowState *state);
extern void tuplewindow_movenext(TupleWindowState *state);
extern bool tuplewindow_gettupleslot(TupleWindowState *state,TupleTableSlot *slot);
extern void tuplewindow_removecurrent(TupleWindowState *state);
extern void tuplewindow_end(TupleWindowState *state);



#endif /* TUPLEWINDOW_H */
