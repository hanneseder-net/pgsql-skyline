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
	const char		   *name;
	SkylineOptionType	option_type;
	SkylineMethod		skyline_method;
} SkylineAnOption;

/* NOTE: keep this list sorted, we are using bsearch */
static SkylineAnOption
skyline_options[] = {
	{ "blocknestedloop"			, SOT_METHOD, SM_BLOCKNESTEDLOOP },
	{ "bnl"						, SOT_METHOD, SM_BLOCKNESTEDLOOP } ,
	{ "ef"						, SOT_PARAM , SM_UNKNOWN },
	{ "efslots"					, SOT_PARAM , SM_UNKNOWN },
	{ "efwindow"				, SOT_PARAM , SM_UNKNOWN },
	{ "efwindowpolicy"			, SOT_PARAM	, SM_UNKNOWN },
	{ "efwindowsize"			, SOT_PARAM , SM_UNKNOWN },
	{ "efwindowslots"			, SOT_PARAM , SM_UNKNOWN },
/*
 * "elimfilter" can not be selected as method on its own, it is the
 * method used for elimination filter nodes.
 */
/*  { "elimfilter"				, SOT_METHOD, SM_ELIMFILTER }, */ 
	{ "materializednestedloop"	, SOT_METHOD, SM_MATERIALIZEDNESTEDLOOP },
	{ "mnl"						, SOT_METHOD, SM_MATERIALIZEDNESTEDLOOP },
	{ "noindex"					, SOT_PARAM	, SM_UNKNOWN },
	{ "presort"					, SOT_METHOD, SM_2DIM_PRESORT },
	{ "ps"						, SOT_METHOD, SM_2DIM_PRESORT },
	{ "sfs"						, SOT_METHOD, SM_SFS },
	{ "slots"					, SOT_PARAM	, SM_UNKNOWN },
	{ "window"					, SOT_PARAM	, SM_UNKNOWN },
	{ "windowpolicy"			, SOT_PARAM , SM_UNKNOWN },
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
 *	Query the SKYLINE OF ... WITH param=xxx list for the param `name' with
 *  data type T_Integer
 *	returns true if value is present.
 */
bool
skyline_option_get_int(List *skyline_of_options, char *name, int *value)
{
	ListCell   *l;

	AssertArg(name != NULL);
	AssertArg(value != NULL);

	/* ensure we only lookup known options */
	Assert(skyline_lookup_option(name) != NULL);

	foreach(l, skyline_of_options)
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

/*
 * skyline_option_get_string
 *
 *  Query the SKYLINE OF ... WITH param=xxx list for the param `name' with
 *  data type T_String
 *  returns true if value is present.
 */
bool
skyline_option_get_string(List *skyline_of_options, char *name, char **value)
{
	ListCell   *l;

	AssertArg(name != NULL);
	AssertArg(value != NULL);

	/* ensure we only lookup known options */
	Assert(skyline_lookup_option(name) != NULL);

	foreach(l, skyline_of_options)
	{
		SkylineOption *option = (SkylineOption *) lfirst(l);

		if (strcmp(option->name, name) == 0)
		{
			A_Const    *arg = (A_Const *) option->value;

			if (!IsA(arg, A_Const))
				elog(ERROR, "unrecognized node type: %d", (int) nodeTag(arg));

			switch (nodeTag(&arg->val))
			{
				case T_String:
					*value = strVal(&arg->val);
					return true;

				default:
					elog(ERROR, "only strings for option `%s' allowed", name);
					return false;
			}
		}
	}

	return false;
}

/*
 * skyline_option_window_policy
 *
 *	Query the SKYLINE OF ... WITH param=xxx list for the param `name' with
 *  data type T_String, return it as "window policy"
 *	returns true if value is present.
 */
bool
skyline_option_get_window_policy(List *skyline_of_options, char *name, TupleWindowPolicy *window_policy)
{
	char	   *window_policy_name;

	if (skyline_option_get_string(skyline_of_options, name, &window_policy_name))
	{
		if (strcmp(window_policy_name, "append") == 0)
			*window_policy = TUP_WIN_POLICY_APPEND;
		else if (strcmp(window_policy_name, "prepend") == 0)
			*window_policy = TUP_WIN_POLICY_PREPEND;
		else if (strcmp(window_policy_name, "entropy") == 0)
			*window_policy = TUP_WIN_POLICY_ENTROPY;
		else if (strcmp(window_policy_name, "random") == 0)
			*window_policy = TUP_WIN_POLICY_RANDOM;
		else
			ereport(ERROR,
				(errcode(ERRCODE_SYNTAX_ERROR),
				 errmsg("unsupported value for \"%s\" \"%s\"",
						name, window_policy_name),
				 errhint("Use \"append\", \"prepend\" or \"entropy\"=\"ranked\".")));
			/* note that ereport(ERROR, ...) does not return */

		return true;
	}
	else
		return false;
}

/*
 * skyline_window_policy_name
 *
 *  FIXME
 */
const char *
skyline_window_policy_name(TupleWindowPolicy window_policy)
{
	switch (window_policy)
	{
	case TUP_WIN_POLICY_APPEND:
		return "append";
	case TUP_WIN_POLICY_PREPEND:
		return "prepend";
	case TUP_WIN_POLICY_ENTROPY:
		return "entropy";
	case TUP_WIN_POLICY_RANDOM:
		return "random";
	default:
		elog(WARNING, "FIXME: skyline window policy `%d' unknown at %s:%d", window_policy, __FILE__, __LINE__);
		return "?";
	}
}

SkylineMethod
skyline_method_forced_by_options(SkylineClause *skyline_clause)
{
	SkylineMethod skyline_method = SM_UNKNOWN;

	ListCell   *l;

	foreach(l, skyline_clause->skyline_of_options)
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
						elog(WARNING, "skyline method `%s' specified more than once", option->name);
					else
						elog(WARNING, "previous skyline method overwritten, now using `%s' for SKYLINE OF", option->name);
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
			elog(WARNING, "unknown option `%s' for SKYLINE OF", option->name);
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
		return list_length(skyline_clause->skyline_of_list);
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
 *
 * FIXME: this can't be used if entropy is used
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

			/* FIXME tuple order is only preserved if window policy "append" is used */
			return false;
		case SM_ELIMFILTER:
			/* 
			 * No matter what tuple window policy is used by the elimination
			 * filter, the relative tuple order is preserved, as the tuple 
			 * is read, compared against the tuples in then window, if it
			 * survives this test, it is passed on.
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
			/*
			 * The term "distinct" should be added by the caller, since 
			 * other all methods can handle distinct as well.
			 */
			return "1dim";		
		case SM_2DIM_PRESORT:
			return "presort";
		case SM_MATERIALIZEDNESTEDLOOP:
			return "mnl";
		case SM_BLOCKNESTEDLOOP:
			return "bnl";
		case SM_SFS:
			return "sfs";
		case SM_ELIMFILTER:
			return "elimfilter";
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

		case SM_ELIMFILTER:
			/*
			 * FIXME: comment this?
			 */
			return true;

		default:
			elog(WARNING, "FIXME: skyline method `%d' unknown at %s:%d", skyline_method, __FILE__, __LINE__);
			return false;
	}
}
