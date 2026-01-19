/*
 * rng_pcg.c - compact PCG32 implementation (XSH-RR)
 *
 * Based on the PCG family by Melissa O'Neill (pcg-random.org).
 * Simple, small, and with very good statistical properties.
 */

#include "rng_pcg.h"
#include <stdint.h>
#include <stdlib.h>

#define PCG32_MULT 6364136223846793005ULL

/* Reentrant seed/init */
void pcg32_srandom_r(pcg32_state_t *rng, uint64_t initstate, uint64_t initseq)
{
    /* As per PCG reference: initialize state to 0, set increment (must be odd),
       advance once, add initstate, advance again. */
    rng->state = 0u;
    rng->inc = (initseq << 1u) | 1u;
    (void)pcg32_random_r(rng);
    rng->state += initstate;
    (void)pcg32_random_r(rng);
}

uint32_t pcg32_random_r(pcg32_state_t *rng)
{
    uint64_t oldstate = rng->state;
    rng->state = oldstate * PCG32_MULT + rng->inc;
    uint32_t xorshifted = (uint32_t)((((oldstate >> 18u) ^ oldstate) >> 27u) & 0xFFFFFFFFu);
    uint32_t rot = (uint32_t)(oldstate >> 59u);
    return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
}

/* Return uniform in [0, bound) without modulo bias using rejection sampling.
 * bound must be > 0.
 */
uint32_t pcg32_boundedrand_r(pcg32_state_t *rng, uint32_t bound)
{
    if (bound == 0) return pcg32_random_r(rng);
    uint32_t threshold = (uint32_t)(-bound) % bound;
    for (;;) {
        uint32_t r = pcg32_random_r(rng);
        if (r >= threshold) return r % bound;
    }
}

/* Produce a double in [0,1). Build a 53-bit integer from two outputs. */
double pcg32_double_r(pcg32_state_t *rng)
{
    uint32_t a = pcg32_random_r(rng);
    uint32_t b = pcg32_random_r(rng);
    uint64_t combined = (((uint64_t)a) << 32) | b;
    uint64_t v = combined >> 11; /* top 53 bits */
    return (double)v / 9007199254740992.0; /* 2^53 */
}

/* Convenience default RNG state. Use thread-local if available. */
#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L) && !defined(__STDC_NO_THREADS__)
/* C11 thread-local */
static _Thread_local pcg32_state_t default_rng = { 0x4d595df4d0f33173ULL, 1442695040888963407ULL };
#elif defined(__GNUC__) || defined(__clang__)
/* gcc/clang thread-local */
static __thread pcg32_state_t default_rng = { 0x4d595df4d0f33173ULL, 1442695040888963407ULL };
#else
/* single global state (not thread-safe) */
static pcg32_state_t default_rng = { 0x4d595df4d0f33173ULL, 1442695040888963407ULL };
#endif

void pcg32_srandom(uint64_t seed)
{
    /* Use a fixed default sequence; seed==0 is allowed and handled */
    pcg32_srandom_r(&default_rng, seed, 0x853c49e6748fea9bULL);
}

uint32_t pcg32_random(void)
{
    return pcg32_random_r(&default_rng);
}

uint32_t pcg32_boundedrand(uint32_t bound)
{
    return pcg32_boundedrand_r(&default_rng, bound);
}

double pcg32_double(void)
{
    return pcg32_double_r(&default_rng);
}