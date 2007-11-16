#ifndef TUPLEWNDHEAP_H
#define TUPLEWNDHEAP_H

#include "executor/tuptable.h"

typedef struct TupleWndHeapState TupleWndHeapState;

extern TupleWndHeapState *tuplewndheap_begin(int maxKBytes, int maxSlots);
extern bool tuplewndheap_has_freespace(TupleWndHeapState *state);
extern void tuplewndheap_puttupleslot(TupleWndHeapState *state, TupleTableSlot *slot, double rank);
extern void tuplewndheap_rewind(TupleWndHeapState *state);
extern bool tuplewndheap_ateof(TupleWndHeapState *state);
extern void tuplewndheap_movenext(TupleWndHeapState *state);
extern bool tuplewndheap_gettupleslot(TupleWndHeapState *state, TupleTableSlot *slot, bool removeit);
extern void tuplewndheap_removecurrent(TupleWndHeapState *state);
extern void tuplewndheap_clean(TupleWndHeapState *state);
extern void tuplewndheap_end(TupleWndHeapState *state);

#endif   /* TUPLEWNDHEAP_H */
