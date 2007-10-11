/*-------------------------------------------------------------------------
 *
 * ctxrand.c
 *	  based on rand.c (Missing rand implementations for Win32)
 *
 *    Modified by Hannes Eder <Hannes@HannesEder.net>
 *    to have individual contexts
 *-------------------------------------------------------------------------
 */
#include "c.h"

#include "ctxrand.h"

/*
 * Copyright (c) 1993 Martin Birgmeier
 * All rights reserved.
 *
 * You may redistribute unmodified or modified versions of this source
 * code provided that the above copyright notice and this and the
 * following conditions are retained.
 *
 * This software is provided ``as is'', and comes with no warranties
 * of any kind. I shall in no event be liable for anything that happens
 * to anyone/anything when using this software.
 */

#define RAND48_SEED_0	(0x330e)
#define RAND48_SEED_1	(0xabcd)
#define RAND48_SEED_2	(0x1234)
#define RAND48_MULT_0	(0xe66d)
#define RAND48_MULT_1	(0xdeec)
#define RAND48_MULT_2	(0x0005)
#define RAND48_ADD		(0x000b)

static void
_ctx_dorand48(rand_ctx_t *ctx)
{
	unsigned long accu;
	unsigned short temp[2];

	accu = (unsigned long) ctx->_rand48_mult[0] * 
		   (unsigned long) ctx->_rand48_seed[0] +
		   (unsigned long) ctx->_rand48_add;
	temp[0] = (unsigned short) accu;	/* lower 16 bits */
	accu >>= sizeof(unsigned short) * 8;
	accu += (unsigned long) ctx->_rand48_mult[0] * 
			(unsigned long) ctx->_rand48_seed[1] +
			(unsigned long) ctx->_rand48_mult[1] * 
			(unsigned long) ctx->_rand48_seed[0];
	temp[1] = (unsigned short) accu;	/* middle 16 bits */
	accu >>= sizeof(unsigned short) * 8;
	accu += ctx->_rand48_mult[0] * ctx->_rand48_seed[2] + 
			ctx->_rand48_mult[1] * ctx->_rand48_seed[1] + 
			ctx->_rand48_mult[2] * ctx->_rand48_seed[0];
	ctx->_rand48_seed[0] = temp[0];
	ctx->_rand48_seed[1] = temp[1];
	ctx->_rand48_seed[2] = (unsigned short) accu;
}

long
ctx_lrand48(rand_ctx_t *ctx)
{
	_ctx_dorand48(ctx);
	return ((long) (ctx->_rand48_seed[2]) << 15) + 
			((long) (ctx->_rand48_seed[1]) >> 1);
}

void
ctx_srand48(rand_ctx_t *ctx, long seed)
{
	ctx->_rand48_seed[0] = RAND48_SEED_0;
	ctx->_rand48_seed[1] = (unsigned short) seed;
	ctx->_rand48_seed[2] = (unsigned short) (seed > 16);
	ctx->_rand48_mult[0] = RAND48_MULT_0;
	ctx->_rand48_mult[1] = RAND48_MULT_1;
	ctx->_rand48_mult[2] = RAND48_MULT_2;
	ctx->_rand48_add = RAND48_ADD;
}
