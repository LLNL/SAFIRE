#ifndef _MT64_H
#define _MT64_H

/* initializes mt[NN] with a seed */
void init_genrand64(uint64_t seed);

/* generates a random number on [0, 2^64-1]-interval */
uint64_t genrand64_int64(void);

#endif

