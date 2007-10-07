/*-------------------------------------------------------------------------
 *
 * randdataset.c
 *
 *
 * Copyright (c) 2007, PostgreSQL Global Development Group
 *
 * Author: Hannes Eder <Hannes@HannesEder.net>
 *         Code based on a work by Donald Kossmann (FIXME)
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

typedef struct
{
	int			dist;
	int			dim;
	int			n;
	int			id;
	random_ctx	rand_ctx;
} randdataset_fctx;

/* forward decl's */
static double rand_equal(random_ctx *rand_ctx, double min, double max);

#define MAXDATASETDIM 20

Datum
pg_rand_dataset(PG_FUNCTION_ARGS)
{
	FuncCallContext	   *funcctx;
	randdataset_fctx   *fctx;

	if (SRF_IS_FIRSTCALL())
	{
		MemoryContext	oldcontext;
		TupleDesc		tupdesc;
		int				nfields;
		int				i;
		long			initseed;

		funcctx = SRF_FIRSTCALL_INIT();
		oldcontext = MemoryContextSwitchTo(funcctx->multi_call_memory_ctx);

		fctx = palloc(sizeof(randdataset_fctx));
		
		fctx->id = 0;
		fctx->dist = PG_GETARG_INT32(0);
		fctx->dim = PG_GETARG_INT32(1);
		fctx->n = PG_GETARG_INT32(2);
		initseed = PG_GETARG_INT32(3);

		ctx_srand48(&(fctx->rand_ctx), initseed);

		if (! (0 < fctx->dim && fctx->dim <= MAXDATASETDIM))
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
					errmsg("dim \"%d\" not in range {0, .., %d}", fctx->dim, MAXDATASETDIM)));

		nfields = 1 /* ID */ + fctx->dim;

		tupdesc = CreateTemplateTupleDesc(nfields, false);
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
	fctx = (randdataset_fctx *) funcctx->user_fctx;

	while (fctx->n--)
	{
		HeapTuple	tuple;
		Datum		values[MAXDATASETDIM+1];
		bool		isnull[MAXDATASETDIM+1];
		int			i;

		memset(isnull, 0, sizeof(isnull));
		
		fctx->id++;

		values[0] = Int32GetDatum(fctx->id);


		for (i = 1; i <= fctx->dim; ++i)
		{
			double rand_value = rand_equal(&(fctx->rand_ctx), 0, 1);
			values[i] = Float8GetDatum(rand_value);
		}


		tuple = heap_form_tuple(funcctx->attinmeta->tupdesc, values, isnull);
/*
		HeapTuple	tuple;
		char	   *values[MAXDATASETDIM+1];
		char		idbuf[32];
		char		field[MAXDATASETDIM+1][32];
		int			i;

		fctx->id++;

		sprintf(idbuf, "%d", fctx->id);
		values[0] = idbuf;

		for (i = 1; i <= fctx->dim; ++i)
		{
			double rand_value = rand_equal(&(fctx->rand_ctx), 0, 1);
			sprintf(field[i], "%8.6f", rand_value);
			values[i] = field[i];
		}

		tuple = BuildTupleFromCStrings(funcctx->attinmeta, values);
*/
		SRF_RETURN_NEXT(funcctx, HeapTupleGetDatum(tuple));
	}

	SRF_RETURN_DONE(funcctx);
}


static double
rand_equal(random_ctx *rand_ctx, double min, double max)
{
	double x = (double) ctx_lrand48(rand_ctx) / LONG_MAX;
	return x * (max - min) + min;
}
