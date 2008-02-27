#ifndef SKYLINE_H
#define SKYLINE_H

#include "nodes/parsenodes.h"
#include "nodes/plannodes.h"
#include "nodes/execnodes.h"

#define SKYLINE_CMP_FIRST_DOMINATES -2
#define SKYLINE_CMP_INCOMPARABLE -1
#define SKYLINE_CMP_ALL_EQ 0
#define SKYLINE_CMP_DIFF_GRP_DIFF 1
#define SYKLINE_CMP_SECOND_DOMINATES 2

extern SkylineMethod skyline_method_forced_by_options(SkylineClause *skyline_clause);
extern SkylineMethod skyline_choose_method(SkylineClause *skyline_clause, bool has_matching_path);
extern int	skyline_get_dim(SkylineClause *skyline_clause);
extern bool skyline_method_preserves_tuple_order(SkylineMethod skyline_method);
extern bool skyline_option_get_int(List *skyline_of_options, char *name, int *value);
extern bool skyline_option_get_string(List *skyline_of_options, char *name, char **value);
extern bool skyline_option_get_window_policy(List *skyline_of_options, char *name, TupleWindowPolicy *window_policy);
extern const char *skyline_method_name(SkylineMethod skyline_method);
extern bool skyline_methode_can_use_limit(SkylineMethod skyline_method);

/* from backend/executor/nodeSkyline.c */
extern int ExecSkylineIsDominating(SkylineState *node, TupleTableSlot *inner_slot, TupleTableSlot *slot);
extern void ExecSkylineCacheCompareFunctionInfo(SkylineState *slstate, Skyline *node);
extern double ExecSkylineRank(SkylineState *node, TupleTableSlot *slot);

#endif   /* SKYLINE_H */
