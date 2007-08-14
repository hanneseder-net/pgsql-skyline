#include "postgres.h"

#include "utils/skyline.h"

typedef enum {
	SOT_UNKNOWN,
	SOT_METHOD,
	SOT_PARAM
} SkylineOptionType;

typedef struct {
	char			   *name;
	SkylineOptionType	option_type;
	SkylineMethod		skyline_method;
} SkylineAnOption;

/* NOTE: keep this list sorted, we are using bsearch */
static SkylineAnOption
skyline_options[] = {
	{ "blocknestedloop"	, SOT_METHOD, SM_BLOCKNESTEDLOOP },
	{ "bnl"				, SOT_METHOD, SM_BLOCKNESTEDLOOP } ,
	{ "nestedloop"		, SOT_METHOD, SM_SIMPLENESTEDLOOP },
	{ "nl"				, SOT_METHOD, SM_SIMPLENESTEDLOOP },
	{ "presort"			, SOT_METHOD, SM_2DIM_PRESORT },
	{ "ps"				, SOT_METHOD, SM_2DIM_PRESORT },
	{ "simplenestedloop", SOT_METHOD, SM_SIMPLENESTEDLOOP },
	{ "slots"			, SOT_PARAM	, SM_UNKNOWN },
	{ "snl"				, SOT_METHOD, SM_SIMPLENESTEDLOOP },
	{ "window"			, SOT_PARAM	, SM_UNKNOWN },
	{ "windowsize"		, SOT_PARAM	, SM_UNKNOWN },
	{ "windowslots"		, SOT_PARAM	, SM_UNKNOWN },
};

static int
cmp_skyline_option(const void *m1, const void *m2)
{
	return strcmp(((SkylineAnOption*)m1)->name, ((SkylineAnOption*)m2)->name);
}

static SkylineAnOption *
skyline_lookup_option(const char * name)
{
	SkylineAnOption key;

	Assert(name != NULL);
	Assert(strlen(name) > 0);

	key.name = name;
	return (SkylineAnOption*)bsearch(&key, skyline_options, lengthof(skyline_options), sizeof(skyline_options[0]), cmp_skyline_option);
}

static SkylineMethod
skyline_method_from_options(SkylineClause* skyline_clause)
{
	SkylineMethod skyline_method = SM_UNKNOWN;

	ListCell	*l;
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
					if (skyline_methode == anoption->skyline_method)
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

SkylineMethod
skyline_choose_method(SkylineClause * skyline_clause)
{
	SkylineMethod	skyline_method = SM_UNKNOWN;
	int				skyline_dim = list_length(skyline_clause->skyline_by_list);

	skyline_method = skyline_method_from_options(skyline_clause);

	if (skyline_method == SM_UNKNOWN)
	{
		/* use a default */
		if (skyline_dim == 1 && !skyline_clause->skyline_distinct)
			skyline_method = SM_1DIM;
		else if (skyline_dim == 1 && skyline_clause->skyline_distinct)
			skyline_method = SM_1DIM_DISTINCT;
		else
			skyline_method = SM_BLOCKNESTEDLOOP;
	}

	/* performe some sanity checks */
	if (skyline_method == SM_2DIM_PRESORT && skyline_dim != 2)
		elog(WARNING, "skyline method `2d with presort' only works correkt for 2 skyline dimensions");

	if (skyline_dim == 1 && !skyline_clause->skyline_distinct && skyline_method != SM_1DIM)
		elog(WARNING, "for 1d skyline the default method is superior");
	else if (skyline_dim == 1 && skyline_clause->skyline_distinct && skyline_method != SM_1DIM_DISTINCT)
		elog(WARNING, "for 1d skyline the default method is superior");

	Assert(skyline_method != SM_UNKNOWN);
	return skyline_method;
}

bool
skyline_method_preserves_tuple_order(SkylineMethod skyline_method)
{
	switch (skyline_method)
	{
		case SM_1DIM:
		case SM_1DIM_DISTINCT:
		case SM_2DIM_PRESORT:
		case SM_SIMPLENESTEDLOOP:
			/* these methods preserve the relative order of the tuples,
			 * so e.g. we may keep the current_pathkeys in the grouping_planner
			 */
			return true;
		case SM_BLOCKNESTEDLOOP:
			/* block nested loop eventually changes the order of the tuples
			 * so we must drop the current_pathkeys
			 */
			return false;
		default:
			/* this we drop it on default */
			elog(WARNING, "FIXME: method `%d' unknown in skyline_method_preserves_tuple_order, assuming false", skyline_method);
			return false;
	}
}
