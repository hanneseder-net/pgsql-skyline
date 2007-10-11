/*-------------------------------------------------------------------------
 *
 * randdataset.c
 *
 *
 * Copyright (c) 2007, PostgreSQL Global Development Group
 *
 * Author:	Hannes Eder <Hannes@HannesEder.net>
 *			code based on [Borzsonyi2001], thanks to 
 *			Donald Kossmann / ETH Zurich for providing the code
 *
 * DESCRIPTION
 *	A random dataset generator similar to the one used in [Borzsonyi2001].
 *
 * [Borzsonyi2001]	Börzsönyi, S.; Kossmann, D. & Stocker, K.:
 *					The Skyline Operator, ICDE 2001, 421--432
 *
 * IDENTIFICATION
 *	  $PostgreSQL: $
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include <limits.h>

#include "catalog/pg_type.h"
#include "funcapi.h"
#include "miscadmin.h"
#include "ctxrand.h"
#include "access\heapam.h"

PG_MODULE_MAGIC;

Datum		pg_rand_dataset(PG_FUNCTION_ARGS);

PG_FUNCTION_INFO_V1(pg_rand_dataset);

typedef void (*generate_fn_t)(rand_ctx_t *, int, Datum *);

typedef struct
{
	char		   *name;
	generate_fn_t	generate_fn;
} dist_info_t;

typedef struct
{
	generate_fn_t	generate_fn;
	int				dim;
	int				n;
	int				id;
	rand_ctx_t		rand_ctx;
} randdataset_fctx_t;

#define MAXDATASETDIM 20

/* forward decl's */
static double rand_equal(rand_ctx_t *rand_ctx, double min, double max);
static void generate_indep(rand_ctx_t *rand_ctx, int dim, Datum *values);
static void generate_corr(rand_ctx_t *rand_ctx, int dim, Datum *values);
static void generate_anti(rand_ctx_t *rand_ctx, int dim, Datum *values);
static int cmp_dist_info(const void *m1, const void *m2);
static dist_info_t *dist_info_lookup(const char *name);

/*
 * the data return will always be NOT NULL so we can init the
 * isnull array here to false
 */
const bool isnull[MAXDATASETDIM+1];

/*
 * pg_rand_dataset
 *
 *	example:
 *	select * from pg_rand_dataset('indep',2,10,0) ds1(id int, d1 float8, d2 float8);
 *	select * from pg_rand_dataset('corr',3,10,0) ds1(id int, d1 float8, d2 float8, d3 float8);
 *	select * from pg_rand_dataset('anti',3,20,0) ds1(id int, d1 float8, d2 float8, d3 float8);
 */
Datum
pg_rand_dataset(PG_FUNCTION_ARGS)
{
	FuncCallContext	   *funcctx;
	randdataset_fctx_t   *fctx;

	if (SRF_IS_FIRSTCALL())
	{
		MemoryContext	oldcontext;
		TupleDesc		tupdesc;
		int				i;

		text		   *disttype_arg;
		int				disttype_len;
		char		   *disttype;
		dist_info_t	   *dist_info;

		funcctx = SRF_FIRSTCALL_INIT();
		oldcontext = MemoryContextSwitchTo(funcctx->multi_call_memory_ctx);

		fctx = palloc(sizeof(randdataset_fctx_t));
		
		fctx->id = 0;

		/* lookup distribution */
		disttype_arg = PG_GETARG_TEXT_P(0);
		disttype_len = VARSIZE(disttype_arg) - VARHDRSZ;
		disttype = palloc(disttype_len + 1);
		memcpy(disttype, VARDATA(disttype_arg), disttype_len);
		disttype[disttype_len] = '\0';

		dist_info = dist_info_lookup(disttype);
		if (dist_info == NULL)
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
					errmsg("distribution type \"%s\" not known", disttype)));

		fctx->generate_fn = dist_info->generate_fn;

		/* setup dimensions */
		fctx->dim = PG_GETARG_INT32(1);
		if (! (0 < fctx->dim && fctx->dim <= MAXDATASETDIM))
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
					errmsg("dim \"%d\" not in range {0, .., %d}", fctx->dim, MAXDATASETDIM)));

		/* number of vectors */
		fctx->n = PG_GETARG_INT32(2);

		/* seed random generator */
		ctx_srand48(&(fctx->rand_ctx), PG_GETARG_INT32(3));

		/*
		 * create tuple descriptor
		 */
		tupdesc = CreateTemplateTupleDesc(1 /* ID */ + fctx->dim, false);
		TupleDescInitEntry(tupdesc, (AttrNumber) 1, "id",
						   INT4OID, -1, 0);

		for (i = 1; i <= fctx->dim; ++i)
		{
			char	fieldname[32];

			snprintf(fieldname, sizeof(fieldname), "d%d", i);
			TupleDescInitEntry(tupdesc, (AttrNumber) (i+1), fieldname,
							   FLOAT8OID, -1, 0);
		}

		funcctx->attinmeta = TupleDescGetAttInMetadata(tupdesc);

		funcctx->user_fctx = fctx;
		MemoryContextSwitchTo(oldcontext);
	}

	funcctx = SRF_PERCALL_SETUP();
	fctx = (randdataset_fctx_t *) funcctx->user_fctx;

	while (fctx->n--)
	{
		HeapTuple	tuple;
		Datum		values[MAXDATASETDIM+1];

		/* set id */
		fctx->id++;
		values[0] = Int32GetDatum(fctx->id);

		/* set vector elements */
		fctx->generate_fn(&(fctx->rand_ctx), fctx->dim, &values[1]);

		/* form tuple */
		tuple = heap_form_tuple(funcctx->attinmeta->tupdesc, values, isnull);

		SRF_RETURN_NEXT(funcctx, HeapTupleGetDatum(tuple));
	}

	SRF_RETURN_DONE(funcctx);
}

/* keep the array sorted by name, so we can use bsearch */
static dist_info_t dist_infos[] =
{
	{ "a"		, generate_anti } ,
	{ "anti"	, generate_anti } ,
	{ "c"		, generate_corr } ,
	{ "corr"	, generate_corr } ,
	{ "i"		, generate_indep },
	{ "indep"	, generate_indep },
};

static int
cmp_dist_info(const void *m1, const void *m2)
{
	return strcmp(((dist_info_t *) m1)->name, ((dist_info_t *) m2)->name);
}

/*
 * dist_info_lookup
 *
 *	lookup distribution and generator function
 */
static dist_info_t *
dist_info_lookup(const char *name)
{
	dist_info_t key;

	Assert(name != NULL);
	Assert(strlen(name) > 0);

	key.name = name;
	return (dist_info_t *) bsearch(&key, dist_infos, lengthof(dist_infos), sizeof(dist_infos[0]), cmp_dist_info);
}

/*
 * rand_equal
 *
 *	returns random value within [min, max]
 */
static double
rand_equal(rand_ctx_t *rand_ctx, double min, double max)
{
	double	x = (double) ctx_lrand48(rand_ctx) / LONG_MAX;

	return x * (max - min) + min;
}

/*
 * rand_peak
 *
 *	returns a random value within [min,max[ as sum of dim equally 
 *	dist. random values
 */
double
rand_peak(rand_ctx_t *rand_ctx, double min, double max, int dim)
{
  double	sum = 0.0;
  int		d;

  for (d = 0; d < dim; d++)
    sum += rand_equal(rand_ctx, 0, 1);
  sum /= dim;
  return sum * (max - min) + min;
}

/*
 * rand_normal
 *
 *	returns a normally distributed random value with expected value
 *	med with the interval ]med-var, med+var[
 */
double
rand_normal(rand_ctx_t *rand_ctx, double med, double var)
{
  return rand_peak(rand_ctx, med - var, med + var, 12);
}

/*
 * is_vector_ok
 *
 *	returns true if all x[0..dim-1] are \in [0..1], false otherwise
 */
static bool
is_vector_ok(int dim, double *x)
{
	while (dim--)
	{
		if (*x < 0.0 || *x > 1.0)
			return false;
		x++;
	}

	return true;
}

/*
 * generate_indep
 *
 *	generates dim independent float8's within [0..1]
 */
static void
generate_indep(rand_ctx_t *rand_ctx, int dim, Datum *values)
{
	int		i;
	for (i = 0; i<dim; ++i)
	{
		double rand_value = rand_equal(rand_ctx, 0, 1);
		values[i] = Float8GetDatum(rand_value);
	}
}

/*
 * generate_corr
 *
 *	generates dim correlated float8's
 */
static void
generate_corr(rand_ctx_t *rand_ctx, int dim, Datum *values)
{
	int		i;
	double	x[MAXDATASETDIM];

	do {
		double	v = rand_peak(rand_ctx, 0, 1, dim);
		double	l = v <= 0.5 ? v : 1.0 - v;

		for (i = 0; i<dim; ++i)
			x[i] = v;

		for (i = 0; i<dim; ++i)
		{
			double	h = rand_normal(rand_ctx, 0, l);
			x[i] += h;
			x[(i + 1) % dim] -= h;
		}
	} while (!is_vector_ok(dim, x));

	for (i = 0; i<dim; ++i)
	{
		values[i] = Float8GetDatum(x[i]);
	}
}

/*
 * generate_anti
 *
 *	generates dim anti-correlated float8's
 */
static void
generate_anti(rand_ctx_t *rand_ctx, int dim, Datum *values)
{
	int		i;
	double	x[MAXDATASETDIM];

	do {
		double	v = rand_normal(rand_ctx, 0.5, 0.25);
		double	l = v <= 0.5 ? v : 1.0 - v;

		for (i = 0; i<dim; ++i)
			x[i] = v;

		for (i = 0; i<dim; ++i)
		{
			double	h = rand_equal(rand_ctx, -l, l);
			x[i] += h;
			x[(i + 1) % dim] -= h;
		}
	} while (!is_vector_ok(dim, x));

	for (i = 0; i<dim; ++i)
	{
		values[i] = Float8GetDatum(x[i]);
	}
}