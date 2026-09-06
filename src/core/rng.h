/*
 * Shared xorshift RNG, the stand-in for GML's random()/irandom_range().
 * Each consumer owns one Rng: gameplay draws from SimWorld.rng (seeded by
 * sim_init), and every cosmetic consumer (fx, butterflies, the house
 * pulse) keeps its own stream so view code can never drain or desync the
 * simulation's randomness.
 */
#ifndef LONGO_RNG_H
#define LONGO_RNG_H

#include <stdint.h>

typedef struct Rng {
    uint32_t state;
} Rng;

static inline uint32_t rng_next(Rng *r)
{
    uint32_t x = r->state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    r->state = x ? x : 0x9e3779b9u;
    return x;
}

/* uniform in [0, max) */
static inline float rng_float(Rng *r, float max)
{
    return (float)(rng_next(r) & 0xFFFFFF) / (float)0x1000000 * max;
}

/* uniform in [lo, hi) */
static inline float rng_range(Rng *r, float lo, float hi)
{
    return lo + rng_float(r, hi - lo);
}

#endif /* LONGO_RNG_H */
