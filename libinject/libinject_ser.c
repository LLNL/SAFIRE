#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include <inttypes.h>
#include <assert.h>
#include <execinfo.h>
#include <string.h>
#include <stdatomic.h>
#include <stdbool.h>
#include "mt64.h"
#include <pthread.h>

// XXX: No need to __attribute__((preserve_all )) caller site saves needed registers
void selMBB(uint64_t *, uint64_t);
void selInst(uint64_t *, uint8_t *);
void doInject(unsigned , uint64_t *, uint64_t *, uint8_t *);

void init() __attribute__((constructor));
void fini() __attribute__((destructor));

// iterator
static uint64_t fi_iterator = 0;
static uint64_t fi_iterator_local = 0;

// FI
static enum type {
    DO_PROFILING,
    DO_REPRODUCTION,
    DO_RANDOM
} action; 

enum {
    INSTRUMENT_BB=0,
    INSTRUMENT_INST=1,
    INSTRUMENT_DETACH=2
};

uint64_t fi_index = 0;
uint64_t op_num = 0;
uint64_t op_size = 0;
unsigned bit_pos = 0;

char inscount_fname[64];
const char *target_fname = "fi-target.txt";
const char *inject_fname = "fi-inject.txt";
FILE *ins_fp, *tgt_fp, *inj_fp;

void selMBB(uint64_t *ret, uint64_t num_insts)
{
    // default: count at BB level
    *ret = INSTRUMENT_BB;

    uint64_t fi_iterator_pre = fi_iterator;
    fi_iterator += num_insts;

    // fi_index > 0 for fault injection
    if( fi_index > 0 ) {
        // Count at Inst level
        if ( fi_index <= fi_iterator_pre ) {
            *ret = INSTRUMENT_DETACH;
            //printf("DETACH fi_index %"PRIu64" < fi_iterator_pre %"PRIu64"\n", fi_index, fi_iterator_pre);
        }
        else if( ( fi_iterator_pre < fi_index ) && ( fi_index <= ( fi_iterator_pre + num_insts ) ) ) {
            *ret = INSTRUMENT_INST;
            fi_iterator_local = fi_iterator_pre;
            //printf("INJECT fi_iterator_local %"PRIu64"\n", fi_iterator_local);
        }
    }
}

void selInst(uint64_t *ret, uint8_t *instr_str)
{
    *ret = 0;
    fi_iterator_local++;
    if( fi_iterator_local == fi_index ) {
        *ret = 1;
        //printf("INJECT fi_iterator_local=%"PRIu64", ins=%s\n", fi_iterator_local, instr_str);
    }
}

void doInject(unsigned num_ops, uint64_t *op, uint64_t *size, uint8_t *bitmask)
{
    assert( ( ( action == DO_REPRODUCTION ) || ( action == DO_RANDOM ) ) && "action is neither DO_REPRODUCTION nor DO_RANDOM!\n");
    unsigned bitflip;
    // Reproduce FI
    if(action == DO_REPRODUCTION) {
        //printf("DO REPRODUCIBLE INJECTION!\n");
        *op = op_num;
        bitflip = bit_pos;

    }
    // Random operands and bit pos FI
    else if(action == DO_RANDOM) {
        //printf("DO RANDOM INJECTION\n");
        *op = genrand64_int64()%num_ops;
        // XXX: size is in bytes, multiply by 8 for bits
        bitflip = (genrand64_int64()%(8*size[*op]));
        op_num = *op;
        op_size = size[*op];
        bit_pos = bitflip;

        inj_fp = fopen(inject_fname, "w");
        fprintf(inj_fp, "fi_index=%"PRIu64", op=%"PRIu64", size=%"PRIu64", bitflip=%u\n", \
                fi_index, op_num, op_size, bit_pos);
        //TODO: for multiple faults, fflush, fclose at last fault
        fclose(inj_fp);
    }

    //printf("op_size %llu size[*op] %llu\n", op_size, size[*op]);
    assert( ( op_size == size[*op] ) && "op_size != size[*op]");

    unsigned i;
    for(i=0; i<op_size; i++)
        bitmask[i] = 0;

    unsigned bit_i = bitflip/8;
    unsigned bit_j = bitflip%8;

    bitmask[bit_i] = (1U << bit_j);

    /*printf("INJECTING FAULT: fi_index=%"PRIu64", op=%"PRIu64", size=%"PRIu64", bitflip=%u\n", \
            fi_index, op_num, op_size, bitflip);*/
}

void init()
{
    // XXX: First try to reproduce a specific injection, next try to read a target instruction for FI. If netheir holds, do a profiling run
    // This is specific injection, including operands, produced after a FI experiment
    if( ( inj_fp = fopen(inject_fname, "r") ) ) {
        // reproduce injection
        //printf("REPRODUCE INJECTION\n");
        int ret = fscanf(inj_fp, "fi_index=%"PRIu64", op=%"PRIu64", size=%"PRIu64", bitflip=%u", \
                &fi_index, &op_num, &op_size, &bit_pos);
        /*fprintf(stdout, "fi_index=%"PRIu64", op=%"PRIu64", size=%"PRIu64", bitflip=%u\n", \
                fi_index, op_num, op_size, bit_pos);*/
        assert(ret == 4 && "fscanf failed to parse input\n");
        assert(fi_index > 0 && "fi_index <= 0!\n");
        assert(op_num >= 0 && "op_num < 0!\n");
        assert(op_size > 0 && "op_size <=0!\n");
        assert(bit_pos >= 0 && "bit_pos < 0!\n");

        fclose(inj_fp);

        action = DO_REPRODUCTION;
    }
    // This is targeted injection, selecting the instruction to run a random experiment
    else if ( ( tgt_fp = fopen(target_fname, "r") ) ) {
        int ret = fscanf(tgt_fp, "fi_index=%"PRIu64"\n", &fi_index);
        assert(ret == 1 && "fscanf failed to parse input\n");
        assert(fi_index > 0 && "fi_index <= 0\n");

        //printf("TARGET fi_index=%"PRIu64"\n", fi_index);

        fclose(tgt_fp);

        action = DO_RANDOM;

        // Initialize the random generator
        uint64_t seed;
        FILE *fp = fopen("/dev/urandom", "r");
        assert(fp != NULL && "Error opening /dev/urandom\n");
        fread(&seed, sizeof(seed), 1, fp);
        init_genrand64(seed);
        assert(ferror(fp) == 0 && "Error reading /dev/urandom\n");
        fclose(fp);

    }
    // This is a profiling run to get the number of target instructions
    else {
        //printf("PROFILING RUN\n");
        action = DO_PROFILING;
    }
}

void fini()
{
    // XXX: This is a profiling run
    if(action == DO_PROFILING) {
        //fprintf(stderr, "Count of dynamic FI target instructions\n");
        sprintf(inscount_fname, "%s", "fi-inscount.txt");
        ins_fp = fopen(inscount_fname, "w");
        assert(ins_fp != NULL && "Error opening inscount file\n");
        fprintf(ins_fp, "fi_index=%"PRIu64"\n", fi_iterator);
        //fprintf(stderr, "fi_index=%"PRIu64"\n", fi_iterator);
        fclose(ins_fp);
    }
}

