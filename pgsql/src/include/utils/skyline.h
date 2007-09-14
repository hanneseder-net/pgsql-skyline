#ifndef SKYLINE_H
#define SKYLINE_H

#include "nodes/parsenodes.h"
#include "nodes/plannodes.h"

extern SkylineMethod skyline_method_forced_by_options(SkylineClause * skyline_clause);
extern SkylineMethod skyline_choose_method(SkylineClause * skyline_clause, bool has_matching_path);
extern int skyline_get_dim(SkylineClause * skyline_clause);
extern bool skyline_method_preserves_tuple_order(SkylineMethod skyline_method);
extern bool skyline_option_get_int(List *skyline_by_options, char *name, int *value);
extern const char *skyline_method_name(SkylineMethod skyline_method);
extern bool skyline_methode_can_use_limit(SkylineMethod skyline_method);


#endif /* SKYLINE_H */
