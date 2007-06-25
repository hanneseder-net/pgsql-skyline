#ifndef TUPLEWINDOW_H
#define TUPLEWINDOW_H

#include "executor/tuptable.h"

typedef struct TupleWindowState TupleWindowState;

extern TupleWindowState* tuplewindow_begin(int maxKBytes);
extern bool tuplewindow_has_freespace(TupleWindowState *state);
extern void tuplewindow_puttupleslot(TupleWindowState *state, TupleTableSlot *slot, int64 timestamp);
extern void tuplewindow_rewind(TupleWindowState *state);
extern bool tuplewindow_ateof(TupleWindowState *state);
extern void tuplewindow_movenext(TupleWindowState *state);
extern int64 tuplewindow_timestampcurrent(TupleWindowState *state);
extern bool tuplewindow_gettupleslot(TupleWindowState *state, TupleTableSlot *slot, bool removeit);
extern void tuplewindow_removecurrent(TupleWindowState *state);
extern void tuplewindow_end(TupleWindowState *state);



#endif /* TUPLEWINDOW_H */