/*
 * rng_pcg.h - PCG32 API for new RNG use
 *
 * Add to your build and include where you want the new RNG.
 * This coexists with the legacy circle_random/circle_srandom.
 */

#ifndef RNG_PCG_H
#define RNG_PCG_H

#include <stdint.h>

/* Reentrant PCG32 state */
typedef struct {
    uint64_t state;
    uint64_t inc;   /* must be odd */
} pcg32_state_t;

/* Reentrant API (recommended for new code) */
void     pcg32_srandom_r(pcg32_state_t *rng, uint64_t initstate, uint64_t initseq);
uint32_t pcg32_random_r(pcg32_state_t *rng);
uint32_t pcg32_boundedrand_r(pcg32_state_t *rng, uint32_t bound); /* uniform in [0,bound) */
double   pcg32_double_r(pcg32_state_t *rng);                      /* uniform in [0,1) */

/* Convenience defaults (not reentrant unless thread-local is available) */
void     pcg32_srandom(uint64_t seed);
uint32_t pcg32_random(void);
uint32_t pcg32_boundedrand(uint32_t bound);
double   pcg32_double(void);

#endif /* RNG_PCG_H */