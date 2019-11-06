#include <stdint.h>
#include "mt64.h"

#define NN 312
#define MM 156
#define MATRIX_A UINT64_C(0xB5026F5AA96619E9)
#define UM UINT64_C(0xFFFFFFFF80000000) /* Most significant 33 bits */
#define LM UINT64_C(0x7FFFFFFF) /* Least significant 31 bits */

/* The array for the state vector */
static uint64_t mt[NN];
/* mti==NN+1 means mt[NN] is not initialized */
static int mti=NN+1;

/* initializes mt[NN] with a seed */
void init_genrand64(uint64_t seed)
{
    mt[0] = seed;
    for (mti=1; mti<NN; mti++)
        mt[mti] =  (UINT64_C(6364136223846793005) * (mt[mti-1] ^ (mt[mti-1] >> 62)) + mti);
}

/* generates a random number on [0, 2^64-1]-interval */
uint64_t genrand64_int64(void)
{
    int i;
    uint64_t x;
    static uint64_t mag01[2]={UINT64_C(0), MATRIX_A};

    if (mti >= NN) { /* generate NN words at one time */

        /* if init_genrand64() has not been called, */
        /* a default initial seed is used     */
        if (mti == NN+1)
            init_genrand64(UINT64_C(5489));

        for (i=0;i<NN-MM;i++) {
            x = (mt[i]&UM)|(mt[i+1]&LM);
            mt[i] = mt[i+MM] ^ (x>>1) ^ mag01[(int)(x&UINT64_C(1))];
        }
        for (;i<NN-1;i++) {
            x = (mt[i]&UM)|(mt[i+1]&LM);
            mt[i] = mt[i+(MM-NN)] ^ (x>>1) ^ mag01[(int)(x&UINT64_C(1))];
        }
        x = (mt[NN-1]&UM)|(mt[0]&LM);
        mt[NN-1] = mt[MM-1] ^ (x>>1) ^ mag01[(int)(x&UINT64_C(1))];

        mti = 0;
    }

    x = mt[mti++];

    x ^= (x >> 29) & UINT64_C(0x5555555555555555);
    x ^= (x << 17) & UINT64_C(0x71D67FFFEDA60000);
    x ^= (x << 37) & UINT64_C(0xFFF7EEE000000000);
    x ^= (x >> 43);

    return x;
}
