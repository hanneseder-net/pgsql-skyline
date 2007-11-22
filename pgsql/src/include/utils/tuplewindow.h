#ifndef TUPLEWINDOW_H
#define TUPLEWINDOW_H

#include "executor/tuptable.h"

typedef struct TupleWindowState TupleWindowState;

extern TupleWindowState *tuplewindow_begin(int maxKBytes, int maxSlots);
extern bool tuplewindow_has_freespace(TupleWindowState *state);
extern void tuplewindow_puttupleslot(TupleWindowState *state, TupleTableSlot *slot, int64 timestamp);
extern void tuplewindow_rewind(TupleWindowState *state);
extern bool tuplewindow_ateof(TupleWindowState *state);
extern void tuplewindow_movenext(TupleWindowState *state);
extern int64 tuplewindow_timestampcurrent(TupleWindowState *state);
extern bool tuplewindow_gettupleslot(TupleWindowState *state, TupleTableSlot *slot, bool removeit);
extern void tuplewindow_removecurrent(TupleWindowState *state);
extern void tuplewindow_clean(TupleWindowState *state);
extern void tuplewindow_end(TupleWindowState *state);
extern void tuplewindow_setinsertrank(TupleWindowState *state, double rank);
extern void tuplewindow_puttupleslotatinsertrank(TupleWindowState *state, TupleTableSlot *slot, int64 timestamp);
extern void tuplewindow_puttupleslot_ranked(TupleWindowState *state, TupleTableSlot *slot, int64 timestamp);

#endif   /* TUPLEWINDOW_H */
