/*-------------------------------------------------------------------------
 *
 * parse_clause.h
 *	  handle clauses in parser
 *
 *
 * Portions Copyright (c) 1996-2008, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * $PostgreSQL: pgsql/src/include/parser/parse_clause.h,v 1.49 2008/01/01 19:45:58 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */
#ifndef PARSE_CLAUSE_H
#define PARSE_CLAUSE_H

#include "parser/parse_node.h"

extern void transformFromClause(ParseState *pstate, List *frmList);
extern int setTargetTable(ParseState *pstate, RangeVar *relation,
					bool inh, bool alsoSource, AclMode requiredPerms);
extern bool interpretInhOption(InhOption inhOpt);
extern bool interpretOidsOption(List *defList);

extern Node *transformWhereClause(ParseState *pstate, Node *clause,
					const char *constructName);
extern Node *transformLimitClause(ParseState *pstate, Node *clause,
					const char *constructName);
extern List *transformGroupClause(ParseState *pstate, List *grouplist,
					List **targetlist, List *sortClause);
extern Node *transformSkylineClause(ParseState *pstate, Node *skylineOfClause,
					List **targetlist, bool resolveUnknown);
extern List *transformSortClause(ParseState *pstate, List *orderlist,
					List **targetlist, bool resolveUnknown);
extern List *transformDistinctClause(ParseState *pstate, List *distinctlist,
					List **targetlist, List **sortClause);

extern List *addAllTargetsToSortList(ParseState *pstate,
					List *sortlist, List *targetlist,
					bool resolveUnknown);
extern List *addTargetToSkylineList(ParseState *pstate, TargetEntry *tle,
					List *skylinelist, List *targetlist,
					SkylineOfDir skylineof_dir, SkylineOfNulls skylineof_nulls,
					List *skylineof_opname, bool resolveUnknown);
extern List *addTargetToSortList(ParseState *pstate, TargetEntry *tle,
					List *sortlist, List *targetlist,
					SortByDir sortby_dir, SortByNulls sortby_nulls,
					List *sortby_opname, bool resolveUnknown);
extern Index assignSortGroupRef(TargetEntry *tle, List *tlist);
extern bool targetIsInSortList(TargetEntry *tle, Oid sortop, List *sortList);
extern bool targetIsInSkylineList(TargetEntry *tle, Oid skylineop, List *skylineList);

#endif   /* PARSE_CLAUSE_H */
