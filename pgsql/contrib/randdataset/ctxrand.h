#ifndef CTXRAND_H
#define CTXRAND_H

typedef struct
{
	unsigned short _rand48_seed[3];
	unsigned short _rand48_mult[3];
	unsigned short _rand48_add;
} rand_ctx_t;

extern long ctx_lrand48(rand_ctx_t *ctx);
extern void ctx_srand48(rand_ctx_t *ctx, long seed);

#endif /* CTXRAND_H */
