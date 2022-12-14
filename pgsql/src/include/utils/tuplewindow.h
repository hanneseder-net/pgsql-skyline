/*-------------------------------------------------------------------------
 *
 * tuplewindow.h
 *	  prototypes for tuplewindow.c
 *
 * Portions Copyright (c) 2008, PostgreSQL Global Development Group
 * Portions Copyright (c) 2007-2008, Hannes Eder
 *
 *
 * DESCRIPTION
 *	  FIXME
 *
 * IDENTIFICATION
 *	  $PostgreSQL: $
 *
 *-------------------------------------------------------------------------
 */
#ifndef TUPLEWINDOW_H
#define TUPLEWINDOW_H

#include "executor/tuptable.h"

typedef struct TupleWindowState TupleWindowState;

typedef enum TupleWindowPolicy
{
	TUP_WIN_POLICY_APPEND,
	TUP_WIN_POLICY_PREPEND,
	TUP_WIN_POLICY_ENTROPY,
	TUP_WIN_POLICY_RANDOM
} TupleWindowPolicy;

extern TupleWindowState *tuplewindow_begin(int maxKBytes, int maxSlots, TupleWindowPolicy policy);
extern bool tuplewindow_has_freespace(TupleWindowState *state);
extern void tuplewindow_setinsertrank(TupleWindowState *state, double rank);
extern void tuplewindow_puttupleslot(TupleWindowState *state, TupleTableSlot *slot, int64 timestamp, bool forced);
extern void tuplewindow_rewind(TupleWindowState *state);
extern bool tuplewindow_ateof(TupleWindowState *state);
extern void tuplewindow_movenext(TupleWindowState *state);
extern int64 tuplewindow_timestampcurrent(TupleWindowState *state);
extern bool tuplewindow_gettupleslot(TupleWindowState *state, TupleTableSlot *slot, bool removeit);
extern void tuplewindow_removecurrent(TupleWindowState *state);
extern void tuplewindow_clean(TupleWindowState *state);
extern void tuplewindow_end(TupleWindowState *state);

#endif   /* TUPLEWINDOW_H */
