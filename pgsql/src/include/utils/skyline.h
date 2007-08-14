#ifndef SKYLINE_H
#define SKYLINE_H

#include "nodes/parsenodes.h"
#include "nodes/plannodes.h"

extern SkylineMethod skyline_choose_method(SkylineClause * skyline_clause);
extern bool skyline_method_preserves_tuple_order(SkylineMethod skyline_method);

#endif /* SKYLINE_H */
