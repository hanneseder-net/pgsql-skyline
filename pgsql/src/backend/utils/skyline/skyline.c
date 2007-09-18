#include "postgres.h"

#include "utils/skyline.h"

typedef enum
{
	SOT_UNKNOWN,
	SOT_METHOD,
	SOT_PARAM
} SkylineOptionType;

typedef struct
{
	char			   *name;
	SkylineOptionType	option_type;
	SkylineMethod		skyline_method;
} SkylineAnOption;

/* NOTE: keep this list sorted, we are using bsearch */
static SkylineAnOption
skyline_options[] = {
	{ "blocknestedloop"			, SOT_METHOD, SM_BLOCKNESTEDLOOP },
	{ "bnl"						, SOT_METHOD, SM_BLOCKNESTEDLOOP } ,
	{ "materializednestedloop"	, SOT_METHOD, SM_MATERIALIZEDNESTEDLOOP },
	{ "mnl"						, SOT_METHOD, SM_MATERIALIZEDNESTEDLOOP },
	{ "presort"					, SOT_METHOD, SM_2DIM_PRESORT },
	{ "ps"						, SOT_METHOD, SM_2DIM_PRESORT },
	{ "sfs"						, SOT_METHOD, SM_SFS },
	{ "slots"					, SOT_PARAM	, SM_UNKNOWN },
	{ "window"					, SOT_PARAM	, SM_UNKNOWN },
	{ "windowsize"				, SOT_PARAM	, SM_UNKNOWN },
	{ "windowslots"				, SOT_PARAM	, SM_UNKNOWN },
};

static int
cmp_skyline_option(const void *m1, const void *m2)
{
	return strcmp(((SkylineAnOption *) m1)->name, ((SkylineAnOption *) m2)->name);
}

static SkylineAnOption *
skyline_lookup_option(const char *name)
{
	SkylineAnOption key;

	Assert(name != NULL);
	Assert(strlen(name) > 0);

	key.name = name;
	return (SkylineAnOption *) bsearch(&key, skyline_options, lengthof(skyline_options), sizeof(skyline_options[0]), cmp_skyline_option);
}

/*
 * skyline_option_get_int
 *
 *	Query the SKYLINE BY ... WITH param=xxx list for the param `name'
 *	returns true if value is present.
 */
bool
skyline_option_get_int(List *skyline_by_options, char *name, int *value)
{
	ListCell   *l;

	AssertArg(name != NULL);
	AssertArg(value != NULL);

	/* ensure we only lookup known options */
	Assert(skyline_lookup_option(name) != NULL);

	foreach(l, skyline_by_options)
	{
		SkylineOption *option = (SkylineOption *) lfirst(l);

		if (strcmp(option->name, name) == 0)
		{
			A_Const    *arg = (A_Const *) option->value;

			if (!IsA(arg, A_Const))
				elog(ERROR, "unrecognized node type: %d", (int) nodeTag(arg));

			switch (nodeTag(&arg->val))
			{
				case T_Integer:
					*value = intVal(&arg->val);
					return true;

				default:
					elog(ERROR, "only integer for option `%s' allowed", name);
					return false;
			}
		}
	}

	return false;
}

SkylineMethod
skyline_method_forced_by_options(SkylineClause *skyline_clause)
{
	SkylineMethod skyline_method = SM_UNKNOWN;

	ListCell   *l;

	foreach(l, skyline_clause->skyline_by_options)
	{
		SkylineOption *option = (SkylineOption *) lfirst(l);
		SkylineAnOption *anoption = skyline_lookup_option(option->name);

		if (anoption != NULL)
		{
			if (anoption->option_type == SOT_METHOD)
			{
				if (skyline_method != SM_UNKNOWN)
				{
					if (skyline_method == anoption->skyline_method)
						elog(WARNING, "skyline method specified more than once", option->name);
					else
						elog(WARNING, "previous skyline method overwritten, now using `%s' for SKYLINE BY", option->name);
				}

				skyline_method = anoption->skyline_method;
			}
			else
			{
				/* ignore them here, they are used in the executor */
			}
		}
		else
		{
			elog(WARNING, "unknown option `%s' for SKYLINE BY", option->name);
		}
	}

	return skyline_method;
}

int
skyline_get_dim(SkylineClause *skyline_clause)
{
	if (skyline_clause == NULL)
		return 0;
	else
		return list_length(skyline_clause->skyline_by_list);
}

SkylineMethod
skyline_choose_method(SkylineClause *skyline_clause, bool has_matching_path)
{
	SkylineMethod skyline_method = SM_UNKNOWN;
	int			skyline_dim = skyline_get_dim(skyline_clause);

	skyline_method = skyline_method_forced_by_options(skyline_clause);

	if (skyline_method == SM_UNKNOWN)
	{
		/* use a default */
		if (skyline_dim == 1 && !skyline_clause->skyline_distinct)
			skyline_method = SM_1DIM;
		else if (skyline_dim == 1 && skyline_clause->skyline_distinct)
			skyline_method = SM_1DIM_DISTINCT;
		else if (skyline_dim == 2 && has_matching_path)
			skyline_method = SM_2DIM_PRESORT;
		else if (skyline_dim >= 2 && has_matching_path)
			skyline_method = SM_SFS;
		else
			skyline_method = SM_BLOCKNESTEDLOOP;
	}

	/* performe some sanity checks */
	if (skyline_method == SM_2DIM_PRESORT && skyline_dim > 2)
		elog(ERROR, "skyline method `2d with presort' only works for <= 2 skyline dimensions");

	if (skyline_dim == 1 && !skyline_clause->skyline_distinct && skyline_method != SM_1DIM)
		elog(WARNING, "for 1d skyline the default method is superior");
	else if (skyline_dim == 1 && skyline_clause->skyline_distinct && skyline_method != SM_1DIM_DISTINCT)
		elog(WARNING, "for 1d skyline the default method is superior");

	Assert(skyline_method != SM_UNKNOWN);
	return skyline_method;
}

/*
 * This function is used to decide in the planner of the current_paths can be kept
 */
bool
skyline_method_preserves_tuple_order(SkylineMethod skyline_method)
{
	switch (skyline_method)
	{
		case SM_1DIM:
		case SM_1DIM_DISTINCT:
		case SM_MATERIALIZEDNESTEDLOOP:
			/*
			 * These methods preserve the relative order of the tuples, so
			 * e.g. we may keep the current_pathkeys in the grouping_planner.
			 */
			return true;
		case SM_2DIM_PRESORT:
			/*
			 * 2d with presort may overall change the order of the tuples but
			 * the filtering step of this methodes preserved them and we are
			 * using an extra sort plan node, so this pathkey can be usind by
			 * an eventually following sort plan node
			 */
			return true;
		case SM_BLOCKNESTEDLOOP:
			/*
			 * Block nested loop eventually changes the order of the tuples so
			 * we must drop the current_pathkeys this is still true even if
			 * the relative order in the tuple window is preserved.
			 */
			return false;
		case SM_SFS:
			/*
			 * The order of the tuples is not changed by the Sort Filter
			 * Skyline method, so the current_pathkeys can be kept.
			 */
			return true;
		default:
			/* 
			 * We drop it on default.
			 */
			elog(WARNING, "FIXME: skyline method `%d' unknown at %s:%d", skyline_method, __FILE__, __LINE__);
			return false;
	}
}


const char *
skyline_method_name(SkylineMethod skyline_method)
{
	switch (skyline_method)
	{
		case SM_UNKNOWN:
			return "unknown";
		case SM_1DIM:
			return "1dim";
		case SM_1DIM_DISTINCT:
			return "1dim distinct";
		case SM_2DIM_PRESORT:
			return "presort";
		case SM_MATERIALIZEDNESTEDLOOP:
			return "mnl";
		case SM_BLOCKNESTEDLOOP:
			return "bnl";
		case SM_SFS:
			return "sfs";
		default:
			return "?";
	}
}

/*
 * skyline_methode_can_use_limit
 *	 Do we have to process all outer at least once tuples befor we
 *	 can return some?
 */
bool
skyline_methode_can_use_limit(SkylineMethod skyline_method)
{
	switch (skyline_method)
	{
		case SM_UNKNOWN:
		case SM_1DIM:
		case SM_1DIM_DISTINCT:
			/* We have to read them all. */
		case SM_BLOCKNESTEDLOOP:
			/*
			 * We have to read them all at least once befor we can be sure
			 * that the first tuple survived.
			 */
			return false;

		case SM_MATERIALIZEDNESTEDLOOP:
			/*
			 * We are cheating here, since the for every tuple the entire
			 * outerplan is read, but once it survived this we can return it.
			 */
		case SM_2DIM_PRESORT:
			/*
			 * If it's not dominated by the current we can return it and make
			 * it the new current.
			 */
		case SM_SFS:
			/*
			 * We can return a tuple if it is not dominated by one in the
			 * tuple window.
			 */
			return true;

		default:
			elog(WARNING, "FIXME: skyline method `%d' unknown at %s:%d", skyline_method, __FILE__, __LINE__);
			return false;
	}
}
