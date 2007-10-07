#ifndef CTXRAND_H
#define CTXRAND_H

typedef struct
{
	unsigned short _rand48_seed[3];
	unsigned short _rand48_mult[3];
	unsigned short _rand48_add;
} random_ctx;

extern long ctx_lrand48(random_ctx *ctx);
extern void ctx_srand48(random_ctx *ctx, long seed);

#endif /* CTXRAND_H */
